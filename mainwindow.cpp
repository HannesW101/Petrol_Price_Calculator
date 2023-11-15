#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QResource>
#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QFile>
#include <QLocale>

//global variable declarations
QStringList gblFuelTypeList,gblltrsPer100kmList;
double U93price,U95price,Dprice,usagePer100km;
QString apiKey = "YOUR-API-KEY-HERE"; // PUT API KEY HERE!!!!!

//function to read values in CSV file
QStringList MainWindow::readCSV(int columnIndex) {
    QStringList values;

    QFile file(":/resources/data/Fuel_Consumption_2000-2022.csv");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList columns = line.split(';');  // Assuming CSV uses ';' as a delimiter

            if (columnIndex < columns.size()) {
                QString value = columns.at(columnIndex).trimmed();
                if (!value.isEmpty()) {
                    values.append(value);
                }
            }
        }
        file.close();
    }

    return values;
}

//setup function for cBcarmake
void MainWindow::setupCbcarmake(Ui::MainWindow *ui) {
    QComboBox *comboBox = ui->cBcarmake;

    // Call function to read CSV file and add items to combobox
    QStringList csvValues = readCSV(1); // input is column index in CSV file

    // Use a QSet with case-insensitive comparison to remove duplicates
    QSet<QString> uniqueValues;
    for (const QString &value : csvValues) {
        uniqueValues.insert(value.toUpper());
    }

    // Convert QSet to QStringList and sort alphabetically
    QStringList sortedValues(uniqueValues.begin(), uniqueValues.end());
    sortedValues.sort(Qt::CaseInsensitive);

    // Add sorted and unique values to the QComboBox
    for (const QString &value : sortedValues) {
        comboBox->addItem(value); // Do not use toUpper() here
    }
}

//setup function for cBcarmodel (populating global fuel list and gblltrsPer100kmList list too)
void MainWindow::setupCbcarmodel(const QString &selectedMake) {
    QComboBox *modelComboBox = ui->cBcarmodel;

    // Clear existing items in cbcarmodel and the global lists
    modelComboBox->clear();
    gblFuelTypeList.clear();
    gblltrsPer100kmList.clear();

    // Call function to read CSV file and add items
    QStringList carYears = readCSV(0);
    QStringList carMakes = readCSV(1);
    QStringList carModels = readCSV(2);
    QStringList carFuelType = readCSV(3);
    QStringList ltrsPer100km = readCSV(4);

    // loop through carMakes and add years+models when the make is the currently selected make
    for (int i = 0; i < carMakes.length(); i++){
        if (selectedMake.toUpper() == carMakes.at(i).toUpper()) {
            modelComboBox->addItem(carYears.at(i).toUpper() + " - " + carModels.at(i).toUpper());

            //adding to global fuel list
            if (carFuelType.at(i)=="X"){
                gblFuelTypeList.append("PETROL UNLEADED 93");
            } else if (carFuelType.at(i)=="Z"){
                gblFuelTypeList.append("PETROL UNLEADED 95");
            } else if (carFuelType.at(i)=="D"){
                gblFuelTypeList.append("DIESEL");
            }

            //adding to global gblltrsPer100kmList
            gblltrsPer100kmList.append(ltrsPer100km.at(i));
        }
    }
}

//function to call Fuel SA API and return the data as a jsondocument
QJsonDocument MainWindow::callAPI(const QString& apiKey) {
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("https://api.fuelsa.co.za/exapi/fuel/current")); //API url

    // Set the API key in the request headers
    request.setRawHeader("key", apiKey.toUtf8());

    QNetworkReply* reply = manager.get(request); //get the reply

    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    //show if there was an error
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "API call failed:" << reply->errorString();
        return QJsonDocument();
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing JSON:" << parseError.errorString();
        return QJsonDocument();
    }

    return jsonResponse;
}

//function to extract the fuel prices from jsondocument
void MainWindow::UpdatePrices(const QJsonDocument& jsonDocument) {
    // Reset global variables
    U93price = U95price = Dprice = 0.0;

    // Extract petrol and diesel arrays from the JSON document
    QJsonArray petrolArray = jsonDocument["petrol"].toArray();
    QJsonArray dieselArray = jsonDocument["diesel"].toArray();

    // Update prices for Unleaded 93 and Unleaded 95 using "Reef" location values
    for (const auto& petrolEntry : petrolArray) {
        QJsonObject petrolObject = petrolEntry.toObject();
        QString location = petrolObject["location"].toString();
        double value = petrolObject["value"].toDouble() / 100.0;  // Convert to double by dividing by 100

        if (location.toUpper() == "REEF") {
            QString octane = petrolObject["octane"].toString();
            if (octane == "93") {
                U93price = value;
            } else if (octane == "95") {
                U95price = value;
            }
        }
    }

    // Update diesel price using the highest value from the diesel section
    for (const auto& dieselEntry : dieselArray) {
        QJsonObject dieselObject = dieselEntry.toObject();
        double value = dieselObject["value"].toDouble() / 100.0;  // Convert to double by dividing by 100

        if (value > Dprice) {
            Dprice = value;
        }
    }
}

//function to calculate trip petrol cost and display in lblPcostOutput
void MainWindow::CalculateTripCost(int index){
    QString currFuelType = ui->lblFuelType->text();
    double fuelPrice = 0.00;

    if (currFuelType == "PETROL UNLEADED 93"){
        fuelPrice = U93price;
    } else if (currFuelType == "PETROL UNLEADED 95"){
        fuelPrice = U95price;
    } else if (currFuelType == "DIESEL"){
        fuelPrice = Dprice;
    }

    // Calculate the cost
    double cost = ((usagePer100km / 100.00) * fuelPrice) * index;
    //format as currency
    QLocale locale;
    QString formattedResult = locale.toCurrencyString(cost);

    //set label lblPcostOutput
    ui->lblPcostOutput->setText(formattedResult);
}

//mainWindow constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //setup cBcarmake
    setupCbcarmake(ui);

    //call API function
    QJsonDocument jsonResponse = callAPI(apiKey);
    if (!jsonResponse.isNull()) {
        // Process the JSON data as needed
        QJsonObject jsonObject = jsonResponse.object();
        qDebug() << "Received JSON:" << jsonObject;
    } else {
        qWarning() << "Failed to retrieve JSON from API.";
    }

    // Update global fuel prices
    UpdatePrices(jsonResponse);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//event for text chanigng in cBcarmake (to setup cBcarmodel)
void MainWindow::on_cBcarmake_currentTextChanged(const QString &arg1)
{
    setupCbcarmodel(ui->cBcarmake->currentText());
}

//event for text changing in cBcarmodel (to change text for fueltypelbl and call function calculateTripCost)
void MainWindow::on_cBcarmodel_currentIndexChanged(int index)
{
    QLabel *lblFuelType = ui->lblFuelType;
    if (index != -1){
        lblFuelType->setText(gblFuelTypeList.at(index));
        usagePer100km = gblltrsPer100kmList.at(index).toDouble();
        CalculateTripCost(ui->sBdistance->value());
    }
    else{
        lblFuelType->setText("");
        ui->lblPcostOutput->setText("");
    }
}

//event for value changing in sBdistance (to change output on lblPcostOutput to cost of trip)
void MainWindow::on_sBdistance_valueChanged(int index)
{
    CalculateTripCost(index);
}


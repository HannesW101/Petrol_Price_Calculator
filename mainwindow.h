#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    void setupCbcarmake(Ui::MainWindow *ui);
    void setupCbcarmodel(const QString &selectedMake);
    QStringList readCSV(int columnIndex);
    QJsonDocument callAPI(const QString& apiKey);
    void UpdatePrices(const QJsonDocument& jsonDocument);
    void CalculateTripCost(int index);

private slots:
    void on_cBcarmake_currentTextChanged(const QString &arg1);
    void on_cBcarmodel_currentIndexChanged(int index);
    void on_sBdistance_valueChanged(int arg1);
};
#endif // MAINWINDOW_H

#include <QCoreApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QString>
int main(int arc, char* argv[])
{
    QCoreApplication app(arc, argv);
    qDebug() << "Available drivers:" << QSqlDatabase::drivers();
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("DRIVER={MariaDB Unicode};"
                       "SERVER=127.0.0.1;"
                       "PORT=3306;"
                       "DATABASE=wms;"
                       "UID=fanyu;"
                       "PWD=862531981;");
    if (db.open()) {
        qDebug() << "Database opened successfully";
        QSqlQuery query(db);
        if (query.exec("select 1")) {
            qDebug() << "Query executed successfully";
        } else {
            qDebug() << "Query execution failed" << db.lastError().text();
        }
    } else {
        qDebug() << "Database opening failed:" << db.lastError().text();
    }
    return 0;
}
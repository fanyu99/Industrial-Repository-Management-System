#include "qsqldatabase.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVector>
#include <QDate>
struct Student {
    int id;
    QString name;
    QString subject;
    int score;
    QDate exam_date;
};
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("DRIVER={MariaDB Unicode};SERVER=127.0.0.1;PORT=3306;DATABASE=test;USER=fanyu;PASSWORD=862531981");
    if (!db.open()) {
        qDebug() << "数据库连接打开失败" << db.lastError().text();
        return 1;
    }
    qDebug() << "数据库连接打开成功";
    QSqlQuery query;
    query.prepare("select * from students where score > 90 and subject='English' order by score desc limit 10");
    if (!query.exec()) {
        qDebug() << query.lastError().text();
        return 1;
    } else {
        QVector<Student> students;
        while (query.next()) {
            int id = query.value("id").toInt();
            QString name = query.value("student_name").toString();
            QString subject=query.value("subject").toString();
            int score=query.value("score").toInt();
            QDate exam_date=query.value("exam_date").toDate();
            students.append({ id,name,subject,score,exam_date });
        }
        qDebug()<<"查询到"<<students.size()<<"条数据" << "最终结果(成绩前十):";
        for (auto stu : students) {
            qDebug() << "id: " << stu.id << " name: " << stu.name << " subject: " << stu.subject << " score: " << stu.score << " exam_date: " << stu.exam_date;
        }
    }
}
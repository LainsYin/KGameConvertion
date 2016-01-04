#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QPushButton;
class QListWidget;
class QLineEdit;
class QMessageBox;
class QHBoxLayout;
class QVBoxLayout;
class QProgressDialog;
class QXmlStreamReader;
class QTextStream;
class QAction;
struct Node
{
    float start;
    int data1;
    int data2;
    int flags;
    float length;
};

namespace Ui {
class MainWindow;
}
class Thread;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void initWidget();

private:
    void parseXML(QString &file_name);
    void parseNode(QXmlStreamReader &xml, QTextStream &out);

private slots:
    void previewFile();
    void outputFile();
    void convertionFile();

    void customContexMenu(const QPoint &pos);
public slots:
    void deleteRow();
private:
    Ui::MainWindow *ui;

    QHBoxLayout *previewLayout;
    QHBoxLayout *pathLayout;
    QHBoxLayout *converLayout;
    QVBoxLayout *vLayout;

    QPushButton *preview;
    QPushButton *output;
    QPushButton *convertion;

    QLineEdit *previewPath;
    QLineEdit *outputPath;

    QListWidget *listWidget;   

    QMenu *menu;
    QAction *action;

    QList<QString> failedFile();

//    QProgressDialog *progressDlg;

//    Thread *thread;
};

#endif // MAINWINDOW_H

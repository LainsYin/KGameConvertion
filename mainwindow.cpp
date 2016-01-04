
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <windows.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QProcessEnvironment>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QLibrary>
#include <QDateTime>
#include <QMessageBox>
#include <QTextCodec>
#include <QDomDocument>

#include <QXmlStreamReader>
#include <QProcess>
#include <QMenu>
#include <QAction>
#include <QListWidgetItem>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setMinimumSize(600, 550);
    this->setWindowTitle(QStringLiteral("XML2KGAME"));
    this->setWindowIcon(QIcon(":/logo.ico"));


    initWidget();

    connect(preview, &QPushButton::clicked, this, &MainWindow::previewFile);
    connect(output, &QPushButton::clicked, this, &MainWindow::outputFile);
    connect(convertion, &QPushButton::clicked, this, &MainWindow::convertionFile);

    connect(listWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::customContexMenu);

    menu = new QMenu();
    action = new QAction("删除", this);
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(action, SIGNAL(triggered()), this, SLOT(deleteRow()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::previewFile()
{
    //QString fileFormat(QStringLiteral("视频文件(*.mpg);;视频文件(*.mp3);;(*.*)"));
    QString fileFormat(QStringLiteral("XML文件(*.xml)"));
    QString document = QProcessEnvironment::systemEnvironment().value("USERPROFILE")+"\\Desktop";
    QStringList pathStrs = QFileDialog::getOpenFileNames(this,
                                                         QStringLiteral("视频转换"),
                                                         document,
                                                         fileFormat
                                                         );

    if(pathStrs.isEmpty())
        return;


    previewPath->setText(pathStrs.last());
    listWidget->addItems(pathStrs);
}

void MainWindow::outputFile()
{
    QString document = QProcessEnvironment::systemEnvironment().value("USERPROFILE")+"\\Desktop";
    QString pathStr = QFileDialog::getExistingDirectory(this,
                                                        QStringLiteral("保存文件路径"),
                                                        document,
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
                                                        );
    if(pathStr.isEmpty())
        return;

    outputPath->setText(pathStr);
}

void MainWindow::convertionFile()
{
    int count = listWidget->count();
    QString destPath = outputPath->text();
    if(destPath.isEmpty()){
        QMessageBox box(QMessageBox::Information, QStringLiteral("提示"), QStringLiteral("输出文件夹不能为空！"));
        box.setStandardButtons(QMessageBox::Ok);
        box.setButtonText(QMessageBox::Ok, QStringLiteral("确定(&Y)"));
        if (box.exec() == QMessageBox::Ok)
            return;
    }

    QStringList failed;
    destPath.append("/");
    for(int i=0; i<count; i++){
        QListWidgetItem *item = listWidget->item(i);
        QString path = item->text();

        if(path.indexOf(".xml") == -1)
            continue;

        parseXML(path);


        QStringList fileNames = path.split("/");
        QString fileName = fileNames.last();

        QString wav = path;
        QString kgame = path;
        wav.replace(".xml", ".wav");
        kgame.replace(".xml", ".kgame");
        QFile wavFile(wav);
        QFile kgameFile(kgame);
        if(wavFile.exists() && kgameFile.exists())
        {
            QString destFilePath = destPath;
            destFilePath.append(fileName);
            QString wavDest = destFilePath;
            QString kgameDest = destFilePath;

            wavDest.replace(".xml", ".wav");
            kgameDest.replace(".xml", ".kgame");
            wavFile.copy(wavDest);

            kgameFile.setFileName(kgameDest);
            if(kgameFile.exists())
                kgameFile.remove();
            wavFile.rename(kgame, kgameDest);

        }
        else
        {
            failed.append(path);
        }
    }


    QStringList list;
    list.append(destPath);
    QProcess::startDetached("KGProject.exe", list);

    ///失败文件提示
    if(!failed.isEmpty()){
        QString str;
        for(int i=0; i<failed.size(); i++){
            str.append(QString("%1\n").arg(failed.at(i)));
        }

        str.append("\n");
        str.append("仔细核对是否缺少相应的.wav文件。");
        QMessageBox::information(NULL, "转换失败提示", str);
    }


}

////
/// \brief ParseXML::parseXML
/// \param file_name
///

void MainWindow::parseXML(QString &file_name)
{
    if(file_name.isEmpty())
        return;

    QFile *fileIn = new QFile(file_name);
    if(!fileIn->open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::information(NULL, QString("title"), QString("open error!"));

        return;
    }

    QString outPath = file_name;
    outPath.replace(".xml", ".kgame");
    QFile fileOut(outPath);
    if(fileOut.exists())
        fileOut.remove();
    if(!fileOut.open(QIODevice::Append | QIODevice::WriteOnly))
        return;
    QTextStream out(&fileOut);

    //QXmlStreamReader操作任何QIODevice.
    QXmlStreamReader xml(fileIn);

    //解析XML，直到结束
    while(!xml.atEnd() && !xml.hasError())
    {
        //读取下一个element.
        QXmlStreamReader::TokenType token = xml.readNext();

        //如果获取的仅为StartDocument,则进行下一个
        if(token == QXmlStreamReader::StartDocument)
        {
            continue;
        }

        //如果获取了StartElement,则尝试读取
        if(token == QXmlStreamReader::StartElement)
        {
            //如果为obj，则对其进行解析
            if(xml.name() == "obj")
            {
//                qDebug() << " xml name : " << xml.name();
                //获取obj属性
                QString classVa;
                QXmlStreamAttributes attributes = xml.attributes();
                if(attributes.hasAttribute("class"))
                    classVa = attributes.value("class").toString();

                if(classVa.compare("MMidiNote") == 0)
                {
                     parseNode(xml, out);
                }
            }
        }
    }
    out << "END";

//    if(xml.hasError())
//    {
//        QMessageBox::information(NULL, QString("parseXML"), xml.errorString());
//    }

    //从reader中删除所有设备、数据，并将其重置为初始状态
    xml.clear();
    fileIn->close();
    fileOut.close();
}

void MainWindow::parseNode(QXmlStreamReader &xml, QTextStream &out)
{
    QList<Node> node;
    //检查是否获取node
    if(xml.tokenType() != QXmlStreamReader::StartElement && xml.name() == "obj")
    {

    }

    //操作下一个
    xml.readNext();
    QString name1 = xml.name().toString();
    QString outStr;
    QStringList tempStr;
    while(!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "obj"))
    {
        if(xml.tokenType() == QXmlStreamReader::StartElement)
        {
            QString name2 = xml.name().toString();
            QString nameV;
            float valueV;
            QXmlStreamAttributes attributes = xml.attributes();
            if(attributes.hasAttribute("value"))
                valueV = attributes.value("value").toFloat();
            if(attributes.hasAttribute("name"))
                nameV = attributes.value("name").toString();
            if(xml.name() == "float")
            {
                if(nameV.compare("Start") == 0){
                    int _int = valueV*1000;
                    tempStr.append(QString::number(_int));
                }

                if(nameV.compare("Length") == 0)
                {
                     int _int = valueV*1000;
                    tempStr.append(QString::number(_int));
                }
            }

            if(xml.name() == "int")
            {
                if(nameV.compare("Data1") == 0)
                {
                    int _data = valueV;
                    tempStr.append(QString::number(_data));
                }
            }

            if(tempStr.size() == 3)
            {
                outStr.append("+ ");
                outStr.append(tempStr.at(0));
                outStr.append(" ");
                outStr.append(tempStr.at(2));
                outStr.append(" ");
                outStr.append(tempStr.at(1));
                out << outStr;
                out << "\n";

                tempStr.clear();
                outStr.clear();
            }
        }



        xml.readNext();
    }
}

void MainWindow::customContexMenu(const QPoint &)
{
    menu->clear();

    menu->addAction(action);
    menu->exec(QCursor::pos());
}

void MainWindow::deleteRow()
{
   QList<QListWidgetItem *> lists = listWidget->selectedItems();
   for(int i=0; i<lists.count(); i++)
   {
       qDebug() << " yes ";
       QListWidgetItem *item = listWidget->takeItem(listWidget->row(lists.at(i)));
       listWidget->removeItemWidget(item);
   }
}


void MainWindow::initWidget()
{
    preview = new QPushButton(this);
    output = new QPushButton(this);
    convertion = new QPushButton(this);

    previewPath = new QLineEdit(this);
    outputPath = new QLineEdit(this);

    listWidget = new QListWidget(this);

    previewLayout = new QHBoxLayout();
    previewLayout->addWidget(previewPath);
    previewLayout->addWidget(preview);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(10);

    pathLayout = new QHBoxLayout();
    pathLayout->addWidget(outputPath);
    pathLayout->addWidget(output);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    pathLayout->setSpacing(10);

    converLayout = new QHBoxLayout();
    converLayout->addStretch();
    converLayout->addWidget(convertion);
    converLayout->addStretch();
    converLayout->setContentsMargins(0, 0, 0, 0);
    converLayout->setSpacing(10);

    vLayout = new QVBoxLayout();
    vLayout->addLayout(previewLayout);
    vLayout->addLayout(pathLayout);
    vLayout->addWidget(listWidget);
    vLayout->addLayout(converLayout);
    vLayout->setContentsMargins(10, 10, 10, 10);
    vLayout->setSpacing(10);

    ui->centralWidget->setLayout(vLayout);

    preview->setFixedSize(90, 36);
    output->setFixedSize(90, 36);
    convertion->setFixedSize(90, 36);

    previewPath->setFixedHeight(36);
    outputPath->setFixedHeight(36);

    preview->setText(QStringLiteral("浏览"));
    QString str(QStringLiteral("输出文件"));
    output->setText(str);
    convertion->setText(QStringLiteral("确认"));

    QString style(" QPushButton{\
                  background-color: rgb(255, 255, 255);\
            border: 1px solid rgb(170, 170, 170);\
        color:rgb(18, 18, 18);\
            font-size:14px;\
            border-radius:5px;\
        }\
        QPushButton:hover{\
            background-color:rgb(255, 255, 255);\
        border: 1px solid rgb(42, 42, 42);\
        color:rgb(42, 42, 42);\
        }\
        \
        QPushButton:pressed{\
            background-color: rgb(255, 146, 62);\
        border: 1px solid rgb(255, 146, 62);\
        color:rgb(255, 255, 255);\
        } \
        \
        QLineEdit{\
        border: 1px solid rgb(170, 170, 170);\
        color:rgb(202, 202, 202);\
            border-radius:5px;\
        }\
        QLineEdit:hover{\
        border: 1px solid rgb(42, 42, 42);\
        color:rgb(202, 202, 202);\
        }\
        QLineEdit:pressed{\
        border: 1px solid rgb(255, 146, 62);\
        color:rgb(88, 88, 88);\
        }\
        QLineEdit:disabled{\
        color:rgb(202, 202, 202);\
        border: 1px solid rgb(170, 170, 170);\
        }\
        QListWidget{\
            background-color:rgb(247, 246, 246);\
            alternate-background-color:rgb(234, 234, 234);\
            font-size:14px;\
        }\
        QListWidget::item{\
        height:40;\
        }\
        "
        );

        this->setStyleSheet(style);
}

        ///写log文件
//        if(failed.count() > 0)
//        {
//            QString filepath = QApplication::applicationDirPath();
//            filepath.append("/convertion.log");
//            QFile logfile(filepath);
//            if(logfile.open(QIODevice::Append | QIODevice::WriteOnly))
//            {

//                for(int i=0; i<failed.count(); i++)
//                {

//                }

//                QString logpath = path;
//                failed.append(logpath);
//                logpath.append(QString(" =  %1\r\n").arg(ret));
//                logfile.write(logpath.toLatin1());

//                logfile.close();
//            }
//         }

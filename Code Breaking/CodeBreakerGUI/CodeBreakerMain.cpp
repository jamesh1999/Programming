#include <regex>
#include <iostream>
#include <vector>
#include "CodeBreakerMain.h"
#include "Globals.h"
#include "ui_CodeBreakerMain.h"
#include "QThread"
#include <QMessageBox>
#include "GlobalSettings.h"
#include "GuessCipher.h"
#include "Vigenere.h"
#include "SimpleSubstitution.h"
#include "Bifid.h"
#include "RailFence.h"

CodeBreakerMain::CodeBreakerMain(QWidget *parent) : QMainWindow(parent), ui(new Ui::CodeBreakerMain)
{
    ui->setupUi(this);

    //Add ciphers
    ui->stackedWidget->addWidget(new Cipher::Bifid());
    ui->stackedWidget->addWidget(new Cipher::RailFence());
    ui->stackedWidget->addWidget(new Cipher::SimpleSubstitution());
    ui->stackedWidget->addWidget(new Cipher::Vigenere());


    //Populate ciphers
    for(int i = 0; i < ui->stackedWidget->count(); ++i)
    {
        Cipher::ICipher* c = qobject_cast<Cipher::ICipher*>(ui->stackedWidget->widget(i));
        ui->cipher->addItem(QString(c->cipher_data.name));
    }

    ui->cipher->setCurrentIndex(0);
}

CodeBreakerMain::~CodeBreakerMain()
{
    delete ui;
}

void CodeBreakerMain::decryptWithCipher(int id)
{
    ui->textBrowser->setText("");

    std::regex e("[^A-Z0-9#]");
    std::string text = ui->ciphertext->toPlainText().toStdString();
    std::transform(text.begin(), text.end(),text.begin(), toupper);

    Cipher::ICipher* cipher = qobject_cast<Cipher::ICipher*>(ui->stackedWidget->widget(id));

    text = std::regex_replace(text, e, "");

    //Check whether cipher is compatible
    std::vector<char> letters;
    for(char c : text)
    {
        if(std::find(letters.begin(), letters.end(), c) == letters.end())
            letters.push_back(c);
    }

    if(letters.size() <= cipher->cipher_data.nchars)
    {
        is_working = true;
        ui->decrypt->setText("Cancel");
        cipher->start(QString::fromStdString(std::regex_replace(text, e, "")));
    }
    else
    {
        QMessageBox warning;
        warning.warning(this, "Warning", "This ciphertext cannot be decrypted using this method! (There are too many unique characters)");
        warning.setFixedSize(500,200);
    }
}

void CodeBreakerMain::on_decrypt_clicked()
{
    if(connected_cipher == -1) return;

    if(is_working)
    {
        is_working = false;
        ui->decrypt->setText("Decrypt");
        qobject_cast<Cipher::ICipher*>(ui->stackedWidget->currentWidget())->cancel();
    }
    else
        decryptWithCipher(ui->cipher->currentIndex());
}

void CodeBreakerMain::on_guess_cipher_clicked()
{
    GuessCipher* pDialog = new GuessCipher(this);

    std::regex e("[^A-Z0-9#]");
    std::string text = ui->ciphertext->toPlainText().toStdString();
    std::transform(text.begin(), text.end(),text.begin(), toupper);
    pDialog->text = std::regex_replace(text, e, "");

    //Get all cipher data
    pDialog->num_ciphers = ui->cipher->count();
    pDialog->cipher_data = new Cipher::CipherData*[ui->cipher->count()];
    for(int i = 0; i < ui->cipher->count(); ++i)
        pDialog->cipher_data[i] =
                &qobject_cast<Cipher::ICipher*>(ui->stackedWidget->widget(i))->cipher_data;
    connect(pDialog, &GuessCipher::guessCipherReturn, this, &CodeBreakerMain::guessCipherReturn);

    pDialog->guessCipher();
    pDialog->exec();
}

void CodeBreakerMain::on_actionGlobal_Settings_triggered()
{
    GlobalSettings* pDialog = new GlobalSettings(this);
    pDialog->exec();
}

void CodeBreakerMain::on_actionClose_triggered()
{
    this->close();
}

void CodeBreakerMain::on_cipher_currentIndexChanged(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
}

void CodeBreakerMain::on_stackedWidget_currentChanged(int index)
{
    if(connected_cipher != -1)
    {
        Cipher::ICipher* c_old = qobject_cast<Cipher::ICipher*>(ui->stackedWidget->widget(connected_cipher));
        QObject::disconnect(dynamic_cast<QObject*>(c_old->getWorker()), SIGNAL(setPlainText(QString)), this, SLOT(setPlainTextOutput(QString)));
        QObject::disconnect(dynamic_cast<QObject*>(c_old->getWorker()), SIGNAL(appendToConsole(QString)), this, SLOT(appendToConsole(QString)));
        QObject::disconnect(dynamic_cast<QObject*>(c_old->getWorker()), SIGNAL(finished()), this, SLOT(finished()));
        QObject::disconnect(dynamic_cast<QObject*>(c_old->getWorker()), SIGNAL(progress(int)), this, SLOT(progress(int)));
    }
    Cipher::ICipher* c = qobject_cast<Cipher::ICipher*>(ui->stackedWidget->widget(index));
    QObject::connect(dynamic_cast<QObject*>(c->getWorker()), SIGNAL(setPlainText(QString)), this, SLOT(setPlainTextOutput(QString)));
    QObject::connect(dynamic_cast<QObject*>(c->getWorker()), SIGNAL(appendToConsole(QString)), this, SLOT(appendToConsole(QString)));
    QObject::connect(dynamic_cast<QObject*>(c->getWorker()), SIGNAL(finished()), this, SLOT(finished()));
    QObject::connect(dynamic_cast<QObject*>(c->getWorker()), SIGNAL(progress(int)), this, SLOT(progress(int)));
    connected_cipher = index;
}

void CodeBreakerMain::guessCipherReturn(int cipher)
{
    ui->cipher->setCurrentIndex(cipher);
    decryptWithCipher(cipher);
}

void CodeBreakerMain::finished()
{
    is_working = false;
    ui->decrypt->setText(QString("Decrypt"));
}

void CodeBreakerMain::progress(int percent)
{

}

void CodeBreakerMain::setPlainTextOutput(QString text)
{
    ui->plaintext->setPlainText(text);
}

void CodeBreakerMain::appendToConsole(QString text)
{
    ui->textBrowser->append(text);
}
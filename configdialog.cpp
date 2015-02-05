#include "configdialog.h"
#include "ui_configdialog.h"

#include <QSettings>
#include <QDir>
#include <QFile>
#include <QDomDocument>

#include "qsvsh/qsvcolordef.h"
#include "qsvsh/qsvcolordeffactory.h"
#include "qsvsh/qsvlangdef.h"
#include "qsvsh/qsvsyntaxhighlighter.h"

class QsvCLangDef : public QsvLangDef {
public:
    QsvCLangDef() : QsvLangDef( ":/qsvsh/qtsourceview/data/langs/cpp.lang" ) {}
};


static QString getBundledStyles(QComboBox *cb, const QString& defaultName) {
    QDir d(":/qsvsh/qtsourceview/data/colors/");
    QString defaultStyle;
    foreach(QString name, d.entryList(QStringList("*.xml"))) {
        QDomDocument doc("mydocument");
        QFile file(d.filePath(name));
        if (file.open(QIODevice::ReadOnly) && doc.setContent(&file)) {
            QDomNodeList itemDatas = doc.elementsByTagName("itemDatas");
            if (!itemDatas.isEmpty()) {
                QDomNamedNodeMap attr = itemDatas.at(0).attributes();
                QString name = attr.namedItem("name").toAttr().value();
                QString desc = attr.namedItem("description").toAttr().value();
                QString style = name + ": " + desc;
                cb->addItem(style, file.fileName());
                if (defaultName == name)
                    defaultStyle = style;
            }
        }
    }
    return defaultStyle;
}

static QString readBundle(const QString& path) {
    QFile f(path);
    if (f.open(QFile::ReadOnly))
        return QString(f.readAll());
    return QString();
}

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog),
    set(new QSettings(this)),
    defColors(0l),
    syntax(0l),
    langCpp(new QsvCLangDef())
{
    ui->setupUi(this);

    ui->plainTextEdit->setPlainText(readBundle(":/help/reference-code-c.txt"));
    QString style = getBundledStyles(ui->styleComboBox, set->value("editor/colorstyle", "Kate").toString());
    if (!style.isEmpty())
        ui->styleComboBox->setCurrentIndex(ui->styleComboBox->findText(style));

    ui->fontSpinBox->setValue(set->value("editor/font/size", 10).toInt());
    ui->fontComboBox->setCurrentFont(QFont(set->value("editor/font/style", "DejaVu Sans Mono").toString()));

    connect(ui->fontComboBox, SIGNAL(activated(int)), this, SLOT(refreshEditor()));
    connect(ui->fontSpinBox, SIGNAL(valueChanged(int)), this, SLOT(refreshEditor()));
    connect(ui->styleComboBox, SIGNAL(activated(int)), this, SLOT(refreshEditor()));

    refreshEditor();
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
    delete langCpp;
    if (defColors)
        delete defColors;
}

void ConfigDialog::on_buttonBox_accepted()
{
    set->setValue("editor/colorstyle", ui->styleComboBox->currentText().split(':').at(0));
    set->setValue("editor/font/size", ui->fontSpinBox->value());
    set->setValue("editor/font/style", ui->fontComboBox->currentFont().family());
}

void ConfigDialog::refreshEditor()
{
    if (defColors)
        delete defColors;
    if (syntax)
        syntax->deleteLater();
    defColors = new QsvColorDefFactory( currentStyle() );
    syntax    = new QsvSyntaxHighlighter( ui->plainTextEdit, defColors, langCpp );
    QPalette p = ui->plainTextEdit->palette();
    p.setColor(QPalette::Base, defColors->getColorDef("dsWidgetBackground").getBackground());
    ui->plainTextEdit->setPalette(p);
    QFont font(ui->fontComboBox->currentFont());
    font.setPointSize(ui->fontSpinBox->value());
    ui->plainTextEdit->setFont(font);
}

const QString ConfigDialog::currentStyle() const
{
    return ui->styleComboBox->itemData(ui->styleComboBox->currentIndex()).toString();
}

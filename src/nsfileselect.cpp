#include "nsfileselect.h"

#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QFileDialog>

NSFileSelect::NSFileSelect(QWidget* parent) : QWidget( parent )
{
    m_filter =  tr("Text files (*.txt *.csv)");
    m_mode = OpenFile;
    leFileName = new QLineEdit(this);
    btnSelect = new QToolButton(this);
    btnSelect->setText("...");
    btnSelect->setMaximumSize( leFileName->sizeHint().height(), leFileName->sizeHint().height() );

    layout = new QHBoxLayout(this);
    layout->addWidget( leFileName );
    layout->addWidget( btnSelect );
    layout->setMargin(0);
    layout->setSpacing(3);

    connect( btnSelect, SIGNAL(clicked()), this, SLOT(selectFile()) );
}

NSFileSelect::~NSFileSelect()
{
}

void NSFileSelect::selectFile()
{
    QString fileName;
    switch (m_mode) {
        case OpenFile: fileName = QFileDialog::getOpenFileName(this, tr("Open file"), leFileName->text(), m_filter ); break;
        case SaveFile: fileName = QFileDialog::getSaveFileName(this, tr("Save file"), leFileName->text(), m_filter ); break;
        case OpenDir: fileName = QFileDialog::getExistingDirectory(this, tr("Select directory"), leFileName->text() ); break;
    }

    if ( !fileName.isEmpty() )
        leFileName->setText(fileName);
}

void NSFileSelect::setFilter( const QString& f )
{
    m_filter = f;
}

void NSFileSelect::setFileName( const QString& fname )
{
    leFileName->setText( fname );
}

QString NSFileSelect::fileName() const
{
    return leFileName->text();
}

QLineEdit *NSFileSelect::fileNameEditor()
{
    return leFileName;
}

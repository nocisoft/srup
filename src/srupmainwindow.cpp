#include "srupmainwindow.h"
#include "ui_srupmainwindow.h"
#include "global.h"

#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>
#include <QCloseEvent>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QTextStream>
#include <QFile>
#include <QProcess>
#include <QFtp>

#define SUBQUERY_TOKEN		"%SUBQUERY%"
#define SUBQUERY_COLUMNNAME	"_subquery_"
#define SUBQUERY_KEY		"%KEY%"
#define SUBQUERY_ERROR		"%ERROR"

SRUPMainWindow::SRUPMainWindow(QWidget *parent) : QWidget(parent), ui(new Ui::SRUPMainWindow)
{
    ui->setupUi(this);
	ui->textEditQuery->installHighlighter();
	ui->textEditSubQuery->installHighlighter();
	ftpState = NotRunning;
	file = 0;
	ftp = 0;
	canceled = false;

	setWindowTitle( APPLICATION_NAME " " APPLICATION_VERSION );
	ui->selectTxtFile->setSelectMode( NSFileSelect::SaveFile );
	ui->selectZipFile->setSelectMode( NSFileSelect::SaveFile );

	connect( ui->btnStart, SIGNAL(clicked()), this, SLOT(startAction()) );
	connect( ui->btnAbort, SIGNAL(clicked()), this, SLOT(abortAction()) );

	connect( this, SIGNAL(totalRows(int)), ui->progressBar, SLOT(setMaximum(int)) );
	connect( this, SIGNAL(rowIsPending(int)), ui->progressBar, SLOT(setValue(int)) );

	readSettings();
	setRunningMode(false);

}

SRUPMainWindow::~SRUPMainWindow()
{
    delete ui;
}

void SRUPMainWindow::closeEvent(QCloseEvent *event)
{
	writeSettings();
	event->accept();
}

void SRUPMainWindow::abortAction()
{
	canceled = true;
	if ( ftp && ftp->hasPendingCommands() )
		ftp->abort();
	setRunningMode(false);
	QMessageBox::warning( this, APPLICATION_NAME, tr("Process canceled.") );
}

void SRUPMainWindow::startAction()
{
	canceled = false;
	setRunningMode(true);
	bool ok = true;

	setStatus( tr("Start actions...") );

	if ( ui->cbRun->isChecked() )
		ok = runQuery();
	if ( ok && ui->cbCreateZIP->isChecked() )
		ok = runZip();
	if ( ok && ui->cbUpload->isChecked() )
		ok = runUpload();

	/*
	if ( ftpState == NotRunning ) {
		if (ok) {
			QMessageBox::information( this, APPLICATION_NAME, tr("Actions succesfully done.") );
		}
		setRunningMode(false);
	}
	*/
	if ( !ok || !ui->cbUpload->isChecked() )
		setRunningMode(false);
}

bool SRUPMainWindow::runQuery()
{
	QString qry = ui->textEditQuery->toPlainText();
	qry.replace(SUBQUERY_TOKEN,QString("0 as %1").arg(SUBQUERY_COLUMNNAME));

	if ( !qry.startsWith( "select", Qt::CaseInsensitive ) ) {
		QMessageBox::warning( this, APPLICATION_NAME, tr("Invalid SQL command: %1").arg(qry) );
		return false;
	}
	setStatus( tr("Connecting to database...") );
	if ( !connectDatabase() )
		return false;

	bool ok = true;
	setStatus( tr("Running query...") );
	{
		QSqlQuery query( QSqlDatabase::database(APPLICATION_NAME) );
		QSqlQuery subquery( QSqlDatabase::database(APPLICATION_NAME) );
		ok = query.exec( qry );
		if ( ok )
			ok = generateTextFile( query, subquery );
		else
			QMessageBox::warning( this, APPLICATION_NAME, query.lastError().text() );
	}
	disconnectDatabase();

	return ok;
}


void SRUPMainWindow::setRunningMode( bool on )
{
	ui->tabWidget->setCurrentWidget( ui->tabUpload );

	ui->btnStart->setEnabled( !on );
	ui->btnAbort->setEnabled( on );
	ui->btnClose->setEnabled( !on );

	ui->tabSettings->setEnabled( !on );
	ui->tabQuery->setEnabled( !on );
	ui->gbTarget->setEnabled( !on );
	ui->gbRun->setEnabled( !on );
	/*
	if ( on ) {
		ui->lblProgress->setVisible(true);
		ui->progressBar->setVisible(true);
		ui->progressBar->reset();
	} else {
		ui->progressBar->setValue( ui->progressBar->maximum() );
		ui->progressBar->setVisible(false);
		ui->lblProgress->setVisible(false);
	}
	*/
	ui->lblProgress->setVisible(on);
	ui->progressBar->reset();
}


bool SRUPMainWindow::runZip()
{
	QString program = ui->leZIP->text().arg( ui->selectZipFile->fileName() ).arg( ui->selectTxtFile->fileName() );
	/*
	QProcess *proc = new QProcess(this);
	connect( proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int,QProcess::ExitStatus)) );
	connect( proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)) );
	setStatus( tr("Running ZIP command: %1").arg(program) );
	proc->start(program);
	*/

	QProcess proc;
	proc.start(program);
	if (!proc.waitForStarted()) {
		QMessageBox::warning( this, APPLICATION_NAME, tr("ZIP command cannot start.\nMay the command syntax is wrong.") );
		return false;
	}

	 //proc.write("Qt rocks!");
	 //proc.closeWriteChannel();

	if (!proc.waitForFinished()) {
		QMessageBox::warning( this, APPLICATION_NAME, tr("ZIP command cannot be finished.") );
		return false;
	}

	return true;
}

bool SRUPMainWindow::runUpload()
{
	QFileInfo fi( ui->selectZipFile->fileName() );
	if ( !fi.exists() ) {
		QMessageBox::warning( this, APPLICATION_NAME, tr("File not found: %1").arg( ui->selectZipFile->fileName() ) );
		return false;
	}
	setStatus( tr("Uploading to FTP server...") );
	if ( file )
		delete file;
	file = new QFile( ui->selectZipFile->fileName() );
	ftpState = Running;

	if (ftp)
		delete ftp;
	ftp = new QFtp(this);

	connect( ftp, SIGNAL(commandStarted(int)), this, SLOT(ftpCommandStarted(int)) );
	connect( ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(ftpCommandFinished(int,bool)) );
	connect( ftp, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(ftpDataTransferProgress(qint64,qint64)) );
	connect( ftp, SIGNAL(done(bool)), this, SLOT(ftpDone(bool)) );

	ftp->connectToHost(ui->leFtpServer->text());
	ftp->login( ui->leUsername->text(), ui->lePassword->text() );
	if ( !ui->leRemoteDir->text().isEmpty() )
		ftp->cd( ui->leRemoteDir->text() );
	ftp->put( file, fi.fileName() );
	ftp->close();

	return true;
}

void SRUPMainWindow::readSettings()
{
	QSettings settings(APPSETTINGS_ORG, APPSETTINGS_APP);
	ui->selectTxtFile->setFileName( settings.value( "targetResultFile", "" ).toString() );
	ui->selectZipFile->setFileName( settings.value( "targetZipFile", "" ).toString() );
	ui->cbRun->setChecked( settings.value( "run", true ).toBool() );
	ui->cbCreateZIP->setChecked( settings.value( "createZIP", true ).toBool() );
	ui->cbUpload->setChecked( settings.value( "upload", true ).toBool() );

	ui->textEditQuery->setPlainText( settings.value( "sqlCommand", "").toString() );
	ui->textEditSubQuery->setPlainText( settings.value( "sqlCommandSub", "").toString() );
	ui->comboDBDriver->setCurrentIndex( settings.value( "DBDriver", 0).toInt() );
	ui->leDBHost->setText(settings.value( "DBHost", "localhost").toString()  );
    ui->leDBDatabase->setText(settings.value( "DBDatabase", "").toString() );
    ui->leDBUser->setText(settings.value( "DBUser", "").toString() );
    ui->leDBPassword->setText(settings.value( "DBPassword", "").toString() );
	ui->leFtpServer->setText(settings.value( "ftpServer", "").toString() );
	ui->leRemoteDir->setText(settings.value( "remoteDir", "").toString() );
	ui->lePassword->setText(settings.value( "password", "").toString() );
	ui->leUsername->setText(settings.value( "username", "").toString() );
	ui->leEncoding->setText(settings.value( "encoding", "ISO8859-2").toString() );
	ui->comboDelim->setCurrentIndex(settings.value( "delimiter", 0 ).toInt() );
	ui->leZIP->setText(settings.value( "zipcommand", "zip %1 %2").toString() );
}

void SRUPMainWindow::writeSettings()
{
	QSettings settings(APPSETTINGS_ORG, APPSETTINGS_APP);
	settings.setValue( "targetResultFile", ui->selectTxtFile->fileName() );
	settings.setValue( "targetZipFile", ui->selectZipFile->fileName() );
	settings.setValue( "run", ui->cbRun->isChecked() );
	settings.setValue( "createZIP", ui->cbCreateZIP->isChecked() );
	settings.setValue( "upload", ui->cbUpload->isChecked() );
	settings.setValue( "sqlCommand", ui->textEditQuery->toPlainText() );
	settings.setValue( "sqlCommandSub", ui->textEditSubQuery->toPlainText() );
	settings.setValue( "DBDriver", ui->comboDBDriver->currentIndex() );
	settings.setValue( "DBHost", ui->leDBHost->text() );
	settings.setValue( "DBDatabase", ui->leDBDatabase->text() );
	settings.setValue( "DBUser", ui->leDBUser->text() );
	settings.setValue( "DBPassword", ui->leDBPassword->text() );
	settings.setValue( "ftpServer", ui->leFtpServer->text() );
	settings.setValue( "remoteDir", ui->leRemoteDir->text() );
	settings.setValue( "password", ui->lePassword->text() );
	settings.setValue( "username", ui->leUsername->text() );
	settings.setValue( "encoding", ui->leEncoding->text() );
	settings.setValue( "delimiter", ui->comboDelim->currentIndex() );
	settings.setValue( "zipcommand", ui->leZIP->text() );

}

char SRUPMainWindow::delimiter() const
{
	int i = ui->comboDelim->currentIndex();
	Delimiter d = (Delimiter)i;
	switch (d) {
		case Tab: return '\t';
		case Semicolon: return ';';
		case Comma: return ',';
		case VerticalBar: return '|';
	}

	return '\0';
}

bool SRUPMainWindow::connectDatabase()
{
	if ( QSqlDatabase::contains(APPLICATION_NAME) )
		QSqlDatabase::removeDatabase(APPLICATION_NAME );

	QSqlDatabase database = QSqlDatabase::addDatabase( ui->comboDBDriver->currentText(), APPLICATION_NAME );
	if ( database.isValid() ) {
		database.setHostName( ui->leDBHost->text() );
		database.setDatabaseName( ui->leDBDatabase->text() );
		database.setUserName( ui->leDBUser->text() );
		database.setPassword( ui->leDBPassword->text() );

		if ( database.open() ) {
		} else {
			QMessageBox::warning( this, APPLICATION_NAME, database.lastError().databaseText() );
			return false;
		}

	} else {
		QMessageBox::warning( this, APPLICATION_NAME, database.lastError().text() );
		return false;
	}

	return true;
}

bool SRUPMainWindow::disconnectDatabase()
{
	QSqlDatabase::removeDatabase(APPLICATION_NAME);
	return true;
}

bool SRUPMainWindow::generateTextFile( QSqlQuery& query, QSqlQuery& subquery )
{
	setStatus( tr("Generating output file...") );
	if ( query.size() == 0 ) {
		QMessageBox::warning( this, APPLICATION_NAME, tr("No data found!") );
		return false;
	}
	//--------------------------------------------------------
	QString subquery_sql = ui->textEditSubQuery->toPlainText();
	bool hasSubQuery = subquery_sql.startsWith( "select", Qt::CaseInsensitive );

	emit totalRows(query.size());
	QFile file( ui->selectTxtFile->fileName() );
	if (file.open(QFile::WriteOnly | QFile::Truncate)) {
		QTextStream out(&file);
		out.setCodec( ui->leEncoding->text().toLocal8Bit() );
		int counter=0;
		const char d = delimiter();
		while (query.next()) {
			QApplication::processEvents();
			counter++;
			emit rowIsPending(counter);
			for ( int fieldNo=0; fieldNo< query.record().count(); ++fieldNo ) {
				if ( fieldNo>0 )
					out << d;

				if ( hasSubQuery && query.record().fieldName(fieldNo) == SUBQUERY_COLUMNNAME ) {
					// SUBQUERY REPLACEMENT
					QString sql(subquery_sql);
					sql.replace(SUBQUERY_KEY, query.value(0).toString() );
					if ( subquery.exec( sql ) ) {
						if ( subquery.next() )
							out << subquery.value(0).toString();
						else
							out << SUBQUERY_ERROR;
					} else {
						QMessageBox::warning( this, APPLICATION_NAME, subquery.lastError().text() + "\n" + sql );
						return false;
					}
				} else {
					out << query.value(fieldNo).toString();
				}
			}
			out << "\r\n";	//CR+LF : ASCII CR+LF (0x0D 0x0A)
			if ( canceled )
				break;
		}
	} else {
		QMessageBox::warning( this, APPLICATION_NAME, tr("Could not open file %1 to write to.").arg(ui->selectTxtFile->fileName()) );
		return false;
	}

	return true;
}

void SRUPMainWindow::setStatus( const QString& msg )
{
	ui->lblProgress->setText( msg );
	QApplication::processEvents();
}

/*
void SRUPMainWindow::error( QProcess::ProcessError )
{
}

void SRUPMainWindow::finished( int,  QProcess::ExitStatus )
{
}
*/
void SRUPMainWindow::ftpCommandFinished( int id, bool error )
{
	if (error)
		setStatus(tr("FTP command finished %1").arg(id) );
	else
		setStatus(tr("FTP error %1").arg(id) );
}

void SRUPMainWindow::ftpCommandStarted( int id )
{
	setStatus(tr("Uploading is in progress %1...").arg(id) );
}

void SRUPMainWindow::ftpDataTransferProgress( qint64 done, qint64 total )
{
	emit totalRows( total );
	emit rowIsPending( done );
}

void SRUPMainWindow::ftpStateChanged( int state )
{
	switch (state) {
		case QFtp::Unconnected: setStatus(tr("There is no connection to the host...")); break;
		case QFtp::HostLookup: setStatus(tr("A host name lookup is in progress....")); break;
		case QFtp::Connecting: setStatus(tr("An attempt to connect to the host is in progress...")); break;
		case QFtp::Connected: setStatus(tr("Connection to the host has been achieved...")); break;
		case QFtp::LoggedIn: setStatus(tr("Connection and user login have been achieved...")); break;
		case QFtp::Closing: setStatus(tr("The connection is closing down...")); break;
	}
}


void SRUPMainWindow::ftpDone( bool error )
{
	if ( canceled )
		return;
	qDebug( "ftpDone %i", error );
	setStatus(tr("FTP action is done.") );
	ftpState = NotRunning;
	//delete ftp;
	//delete file;
	if ( error )
		QMessageBox::warning( this, APPLICATION_NAME, tr("FTP upload error.") );
	else
		QMessageBox::information( this, APPLICATION_NAME, tr("File %1 has succesfully uploaded.").arg( ui->selectZipFile->fileName()) );

	setRunningMode( false );
}

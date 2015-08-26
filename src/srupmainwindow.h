#ifndef SRUPMAINWINDOW_H
#define SRUPMAINWINDOW_H

#include <QWidget>
#include <QProcess>

class QCloseEvent;
class QSqlQuery;
class QFtp;
class QFile;

namespace Ui
{
    class SRUPMainWindow;
}

class SRUPMainWindow : public QWidget
{
    Q_OBJECT
public:
    SRUPMainWindow(QWidget *parent = 0);
    ~SRUPMainWindow();

	enum Delimiter { Tab=0, Semicolon, Comma, VerticalBar };
	enum FtpState { NotRunning=0, Running=0 };

signals:
	void totalRows(int);
	void rowIsPending(int);

private slots:
	void startAction();
	void abortAction();
	/*
	void error( QProcess::ProcessError );
	void finished( int exitCode, QProcess::ExitStatus);
	*/
	void ftpCommandFinished( int id, bool error );
	void ftpCommandStarted( int id );
	void ftpDataTransferProgress( qint64 done, qint64 total );
	void ftpStateChanged( int state );
	void ftpDone( bool error );

protected:
	void closeEvent(QCloseEvent *event);

private:
    Ui::SRUPMainWindow *ui;
	bool isRunning;
	bool canceled;
	QFtp *ftp;
	QFile *file;
	FtpState ftpState;

	bool runQuery();
	bool runZip();
	bool runUpload();

	bool connectDatabase();
	bool disconnectDatabase();
	bool generateTextFile( QSqlQuery&, QSqlQuery& );
	void setRunningMode( bool );
	void readSettings();
	void writeSettings();
	void refreshButtons();
	void setStatus( const QString& msg );

	char delimiter() const;

};

#endif // SRUPMAINWINDOW_H

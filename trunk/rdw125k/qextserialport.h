/*
 QextSerialPort: (c) Stefan Sander (stefan-sander@users.sourceforge.net)

 On public domain
 */
#ifndef _QEXTSERIALPORT_H_
#define _QEXTSERIALPORT_H_

#include <QIODevice>

#ifdef Q_WS_WIN
	#include <windows.h>
#else
	#include <unistd.h>
	#include <termios.h>
	#include <sys/ioctl.h>

	/*ensure handling of CRTSCTS constant*/
	#ifdef CNEW_RTSCTS
		#ifndef CRTSCTS
			#define CRTSCTS CNEW_RTSCTS
		#endif
	#else
		#ifndef CRTSCTS
			#define CRTSCTS     0
		#endif
	#endif
#endif

#ifdef _QESP_LOGTRACE_
	#define QESP_LOGTRACE(i) qWarning("["+Name.toAscii()+"] QextSerialPort::"+i);
#else
	#define QESP_LOGTRACE(i)
#endif

class QFile;

class QextSerialPort
 : public QIODevice
{
public:
	enum BaudRateType
	{
		BAUD50,      //POSIX ONLY
		BAUD75,      //POSIX ONLY
		BAUD110,
		BAUD134,     //POSIX ONLY
		BAUD150,     //POSIX ONLY
		BAUD200,     //POSIX ONLY
		BAUD300,
		BAUD600,
		BAUD1200,
		BAUD1800,    //POSIX ONLY
		BAUD2400,
		BAUD4800,
		BAUD9600,
		BAUD14400,   //WINDOWS ONLY
		BAUD19200,
		BAUD38400,
		BAUD56000,   //WINDOWS ONLY
		BAUD57600,
		BAUD76800,   //POSIX ONLY
		BAUD115200,
		BAUD128000,  //WINDOWS ONLY
		BAUD256000   //WINDOWS ONLY
	};

	enum DataBitsType
	{
		DATA_5,      //only for compatibility
		DATA_6,
		DATA_7,
		DATA_8
	};

	enum ParityType
	{
		PAR_NONE,
		PAR_ODD,
		PAR_EVEN,
		PAR_MARK,    //WINDOWS ONLY
		PAR_SPACE    //WINDOWS directly, POSIX simulated
	};

	enum StopBitsType
	{
		STOP_1,
		STOP_1_5,    //WINDOWS ONLY
		STOP_2
	};

	enum FlowType
	{
		FLOW_OFF,
		FLOW_HARDWARE,
		FLOW_XONXOFF
	};

	/*structure to contain port settings*/
	struct PortSettings
	{
		BaudRateType BaudRate;
		DataBitsType DataBits;
		ParityType Parity;
		StopBitsType StopBits;
		FlowType FlowControl;
		unsigned long Timeout_Sec;
		unsigned long Timeout_Millisec;
	};

	QextSerialPort( const QString& name = "", QObject* parent = 0 );
	virtual ~QextSerialPort();

	virtual bool isSequential() const;

	virtual bool open( QIODevice::OpenMode mode = 0 );
	virtual void close();

#ifdef _QESP_LOGTRACE_
	virtual qint64 pos() const;  // non-pure virtual
	virtual qint64 size() const;  // non-pure virtual
#endif
	virtual bool seek( qint64 );
#ifdef _QESP_LOGTRACE_
	virtual bool atEnd() const;  // non-pure virtual
	virtual bool reset();  // non-pure virtual
#endif

	virtual qint64 bytesAvailable() const;
#ifdef _QESP_LOGTRACE_
	virtual qint64 bytesToWrite() const;  // non-pure virtual

	qint64 read( char *data, qint64 maxlen );
	QByteArray read( qint64 maxlen );
	QByteArray readAll();
	qint64 readLine( char *data, qint64 maxlen );
	QByteArray readLine( qint64 maxlen = 0 );
	virtual bool canReadLine() const;  // non-pure virtual

	qint64 write( const char *data, qint64 len );
	qint64 write( const QByteArray &data );

	qint64 peek( char *data, qint64 maxlen );
	QByteArray peek( qint64 maxlen );

	virtual bool waitForReadyRead( int msecs );  // non-pure virtual
	virtual bool waitForBytesWritten( int msecs );  // non-pure virtual

	void ungetChar(char c);
	bool putChar(char c);
	bool getChar(char *c);
#endif

protected:
	virtual qint64 readData(char *data, qint64 maxlen);
#ifdef _QESP_LOGTRACE_
	virtual qint64 readLineData(char *data, qint64 maxlen);
#endif
	virtual qint64 writeData(const char *data, qint64 len);

public:
	bool flush();

	void setName( const QString& name );
	QString name();

	virtual void setBaudRate( BaudRateType );
	virtual void setDataBits( DataBitsType );
	virtual void setParity( ParityType );
	virtual void setStopBits( StopBitsType );
	void setFlowControl( FlowType );
	void setTimeout( unsigned long=0, unsigned long=0 );

	BaudRateType baudRate() const { return Settings.BaudRate; };
	DataBitsType dataBits() const { return Settings.DataBits; };
	ParityType parity() const { return Settings.Parity; };
	StopBitsType stopBits() const { return Settings.StopBits; };
	FlowType flowControl() const { return Settings.FlowControl; };

	void setRts( bool set = TRUE );
	void setDtr( bool set = TRUE );

	bool cts() const;
	bool dsr() const;
	bool dcd() const;
	bool ri() const;

private:
	QextSerialPort( const QextSerialPort& sp );
	QextSerialPort& operator=( const QextSerialPort& sp );

protected:
#ifdef Q_WS_WIN
	HANDLE Win_Handle;
	DCB Win_CommConfig;
	COMMTIMEOUTS Win_CommTimeouts;
#else
	QFile* Posix_File;
	struct termios Posix_CommConfig;
#endif

	QString Name;
	PortSettings Settings;
};

#endif

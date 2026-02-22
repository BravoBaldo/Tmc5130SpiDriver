//cHIDAPI.h
#ifndef _CHIDAPI_H
#define _CHIDAPI_H

#include "wx/wx.h"
#include "hidapi.h"

#define GETMILLISECS(D)	(((wxDateTime::UNow() - (D)).GetMilliseconds()).ToLong())
#define INITTIMER(D)	((D) = wxDateTime::UNow())

struct sVID_PID {
	wxString		m_Name = "";
	unsigned short	vendor_id = 0;
	unsigned short	product_id = 0;
	wchar_t*		serial_number = nullptr;
	void	Clear	(void){
							m_Name			= "";
							vendor_id		= 0;
							product_id		= 0;
							serial_number	= NULL;
						}
	const	sVID_PID & operator=	( const sVID_PID & Src){
								m_Name			= Src.m_Name;
								vendor_id		= Src.vendor_id;
								product_id		= Src.product_id;
								serial_number	= Src.serial_number;
								return(*this);
							};

};

class cHIDAPI {
public:
					cHIDAPI			( void  );
					~cHIDAPI		( void );
	bool			Open			( const sVID_PID & vidPid );
	static bool		IsOpenable		( const sVID_PID & vidPid );
	bool			IsOpened		( void );
	int				Read			( void );
	
	size_t			GetAnswerLen	( void )	{ return m_BuffIn.GetDataLen(); };
	wxString		GetBuffAsString	( void );
	wxString		GetBuffAsHex	( void );
	void*			GetBuffer		( void ) const	{ return m_BuffIn.GetData(); }
	void			ResetBuffer		( void )		{ m_BuffIn.SetDataLen(0); }
#if defined(ADDED_BY_ME)
	bool			IsBlocking		(void);
	unsigned long	GetLastErrorNum	(void);
	size_t			GetBufferLen	(void);
	bool			IsReadPending	(void);
#endif
	wxString		GetLastError	( void );
	int				Write			( const unsigned char *data, size_t length );
	bool			Close			( void );
	static bool		Enumerate		( wxArrayString& strList );

	int				Send_NoWait		( const wxString & S );
	int				Write_NoWait	( const unsigned char *data, size_t length );
	//----------------------------------
	wxString		GetInfo			(byte Type);
	wxString		ManufacturerGet	(void)	{ return GetInfo(0); }	// Read the Manufacturer String
	wxString		ProductGet		(void)	{ return GetInfo(1); }// Read the Product String
	wxString		SerialNumberGet	(void)	{ return GetInfo(2); }// Read the Serial Number String


private:
	void	Reset	( void );
	wxMemoryBuffer	m_BuffIn;
	hid_device*		m_hid_device;
};


#endif // _CHIDAPI_H

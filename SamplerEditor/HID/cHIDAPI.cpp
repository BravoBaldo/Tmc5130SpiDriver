/*
*/
#include "stdwx.h"
#include "cHIDAPI.h"

void cHIDAPI::Reset( void ){
	m_hid_device = NULL;
}

bool cHIDAPI::Close( void ){
	hid_close( m_hid_device );
	Reset();	//m_hid_device = NULL;
	return true;
};

cHIDAPI::cHIDAPI( void ){
	Reset();	//m_hid_device = NULL;
}

cHIDAPI::~cHIDAPI( void ){
	Close();	//m_hid_device = NULL;
}

bool cHIDAPI::Open( const sVID_PID & vidPid){
	Close();	//m_hid_device = NULL;
	m_hid_device = hid_open( vidPid.vendor_id,  vidPid.product_id,  vidPid.serial_number );
	
	if(m_hid_device)
		hid_set_nonblocking( m_hid_device, 1);
	return (m_hid_device != NULL);
}

bool cHIDAPI::IsOpened( void ) {
	return (m_hid_device != NULL);
}

int cHIDAPI::Read( void ){
	m_BuffIn.SetDataLen(0);
	if(m_hid_device==NULL)
		return -1;
	unsigned char buf[256];
	int nBytes = hid_read( m_hid_device, buf, sizeof(buf));
	if ( nBytes>0 )
		m_BuffIn.AppendData( (const void *)buf, nBytes );
	else if(nBytes<0)
		Close();	//Disconnection detected

	return nBytes;
}

wxString cHIDAPI::GetBuffAsString( void ){
	wxString RetVal = "";
	size_t L = m_BuffIn.GetDataLen();
	if( L>0) {
		char* Ptr = (char*)m_BuffIn.GetData();
		Ptr[L] = '\0';
		RetVal = Ptr;
	}
	return RetVal;
}

wxString cHIDAPI::GetBuffAsHex( void ){
	wxString RetVal = "";
	size_t L = m_BuffIn.GetDataLen();
	byte* Ptr = (byte*)m_BuffIn.GetData();
	for(size_t i=0; i<L; i++ ){
		RetVal += wxString::Format("%02x ", (unsigned int)Ptr[i] );
	}
	RetVal += wxString::Format("(%d bytes)", (unsigned int)L );
	return RetVal;
}

bool IsRunning = false;

int cHIDAPI::Write( const unsigned char *data, size_t length ){
	if(m_hid_device==NULL)
		return -1;
	int RetVal;
	IsRunning=true;
	RetVal = hid_write( m_hid_device, data, length);	
	IsRunning = false;
	return RetVal;
}

int cHIDAPI::Write_NoWait( const unsigned char *data, size_t length ){
	unsigned char Msg[1000] = "_                         ";
	size_t MaxLen = wxMin(sizeof(Msg)-1, length);
	for(size_t i=0; i<MaxLen; i++){
		Msg[i+1] = data[i];
	}	
	Msg[0] = '\0';	//Must be 0
	return Write( Msg, MaxLen+1 );
}

int cHIDAPI::Send_NoWait( const wxString & ToSend ){
	return Write_NoWait(ToSend.c_str().AsUnsignedChar(), ToSend.Length() );
}

wxString cHIDAPI::GetLastError(void) {
	wxString RetVal = "";
	if (m_hid_device) {
		RetVal = (wchar_t*)hid_error(m_hid_device);
	}
	return RetVal;
}


#if defined(ADDED_BY_ME)
bool cHIDAPI::IsBlocking( void ){
	if(m_hid_device){
		return (hid_IsBlocking(m_hid_device)==TRUE);
	}
	return false;
}

unsigned long cHIDAPI::GetLastErrorNum( void ){
	if(m_hid_device){
		return hid_GetLastErrorNum(m_hid_device);
	}
	return 0;
};

bool cHIDAPI::IsReadPending( void ){
	if(m_hid_device)
		return hid_IsReadPending(m_hid_device)==TRUE;
	return false;
}

size_t cHIDAPI::GetBufferLen(void) {
	if (m_hid_device) {
		return m_hid_device->input_report_length;
//		return hid_GetInputReportLength(m_hid_device);
		//hid_dev_* Hidptr = (hid_dev_*)m_hid_device;
		//return Hidptr->input_report_length;
	}
	return 0;
};
#endif


bool cHIDAPI::Enumerate( wxArrayString& strList ){
	strList.Clear();
	hid_device_info*	devices = hid_enumerate(0x0, 0x0);
	hid_device_info*	cur_dev = devices;	
	while (cur_dev) {
		wxString usage_str;
		usage_str.Printf(" (usage: %04x:%04x) ", cur_dev->usage_page, cur_dev->usage);	//Windows/Mac only

		wxString s;
		s.Printf("%04hx:%04hx -", cur_dev->vendor_id, cur_dev->product_id);
			if(cur_dev->manufacturer_string)
				s << wxString(" ") + cur_dev->manufacturer_string;
			if(cur_dev->product_string)
				s << wxString(" ") + cur_dev->product_string;
			s += usage_str;
		strList.Add(s);
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devices);
	return true;
}

bool cHIDAPI::IsOpenable( const sVID_PID & vidPid){
	hid_device_info *devs = hid_enumerate(vidPid.vendor_id, vidPid.product_id);
	if (devs == NULL) {
		hid_exit();
		return false;
	}
	bool IsSerial = true;
	if (vidPid.serial_number) {
		IsSerial = wcscmp(vidPid.serial_number, devs->serial_number) == 0;
	}
	hid_free_enumeration(devs);
	hid_exit();
	return IsSerial;
}

wxString cHIDAPI::GetInfo(byte Type){
	wxString Ret;
	int rs=-1;
	wchar_t wstr[255];
	if(m_hid_device){
		switch(Type){
			default:
			case 0:	rs = hid_get_manufacturer_string	(m_hid_device, wstr, sizeof(wstr));	break;
			case 1: rs = hid_get_product_string			(m_hid_device, wstr, sizeof(wstr));	break;
			case 2: rs = hid_get_serial_number_string	(m_hid_device, wstr, sizeof(wstr));	break;
		}
		if( rs==0 ){
			Ret = wstr;
		}
	}
	return Ret;
}

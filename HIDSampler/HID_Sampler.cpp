#include "HID_Sampler.h"
#include "../SysSampler.h"

USBHID HID;

static const uint8_t report_descriptor[] = {
  0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
  0x09, 0x01,        // Usage (Vendor Usage 1)
  0xA1, 0x01,        // Collection (Application)
#if defined(USE_REPORT_ID)
  0x85, 0x01,        // Report ID (1)
#endif
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x40,        //   Usage Maximum (0x40) - 64 usages
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size: 8-bit
  0x95, 0x40,        //   Report Count (64)

  0x81, 0x00,        //   Input (Data, Array, Abs)
  0x19, 0x01,        //   Usage Minimum (0x01) 
  0x29, 0x40,        //   Usage Maximum (0x40) 
  0x91, 0x00,        //   Output (Data, Array, Abs)
  0xC0               // End Collection
};

SamplerHIDDevice::SamplerHIDDevice(void) {  //Ctor
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    HID.addDevice(this, sizeof(report_descriptor));
  }
}

void SamplerHIDDevice::begin(void) {
  HID.begin();
}

uint16_t SamplerHIDDevice::_onGetDescriptor(uint8_t *buffer) {
  Serial.printf("Descriptor Got\n");

  memcpy(buffer, report_descriptor, sizeof(report_descriptor));
  return sizeof(report_descriptor);
}

void SamplerHIDDevice::_onOutput(uint8_t report_id, const uint8_t* data, uint16_t len) {
  if(m_LogRx){
    Serial.printf("Received %d bytes (ID: %d):\n", len, report_id);
    Serial.print("HEX..:");
    for(int i = 0; i < len; i++) {
      Serial.printf("%02X ", data[i]);
    }
    Serial.println();

    Serial.print("ASCII:");
    for(int i = 0; i < len; i++) {
      Serial.printf(" %c ", (data[i] >= 32 && data[i] <= 126) ? (char)data[i] : '.'  );
    }
    Serial.println();
  }

  if(m_cbExec) {
    if(m_LogRx) Serial.println("Calling m_cbExec");
    m_cbExec(data, len);
  }else{
    if(m_LogRx){
      Serial.println("m_cbExec NOT defined");
      Serial.println("sending phrase");
    }
    const char* frase = "Bellissimo il mondo oggigiorno!";
    SendBuffer((uint8_t*)frase, strlen(frase));    
  }
}

void SamplerHIDDevice::SendBuffer(uint8_t* Buffer, size_t len, uint8_t ReportId){
  memset(m_OutBuffer, 0, sizeof(m_OutBuffer));
  memcpy( (void *)m_OutBuffer, (const void *)Buffer, min(len, sizeof(m_OutBuffer))); // Limit to max

  if (HID.ready()) {
    HID.SendReport(ReportId, m_OutBuffer, sizeof(m_OutBuffer));
    Serial.printf("Sent buffer (%d bytes or %d)!\n\n", sizeof(m_OutBuffer), len);
  }
}

void SamplerHIDDevice::Setup(void){
  Serial.println("Init HID Sampler...");
  begin();

  USB.VID(UVID);
  USB.PID(UPID);
  USB.manufacturerName("P.S. Elettronica - Italy - Nazar HID");
  USB.productName("HID Nazar Interface");
  USB.serialNumber("Sampler2026");
  
  USB.begin();
}


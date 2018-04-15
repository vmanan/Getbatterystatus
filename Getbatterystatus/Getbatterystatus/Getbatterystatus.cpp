// Getbatterystatus.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
// Windows Header Files:
#include <windows.h>
#include <comdef.h>

// C RunTime Header Files
#include <stdlib.h>
#include <strsafe.h>
#include  <Poclass.h>
#include <Setupapi.h>

#include <initguid.h>
//#define INITGUID

#include<devguid.h>

//#include <batclass.h>

DWORD GetBatteryState()
 {
#define GBS_HASBATTERY 0x1
#define GBS_ONBATTERY  0x2
  // Returned value includes GBS_HASBATTERY if the system has a 
  // non-UPS battery, and GBS_ONBATTERY if the system is running on 
  // a battery.
  //
  // dwResult & GBS_ONBATTERY means we have not yet found AC power.
  // dwResult & GBS_HASBATTERY means we have found a non-UPS battery.

  DWORD dwResult = GBS_ONBATTERY;

  // IOCTL_BATTERY_QUERY_INFORMATION,
  // enumerate the batteries and ask each one for information.

  HDEVINFO hdev =
            SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, 
                                0, 
                                0, 
                                DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (INVALID_HANDLE_VALUE != hdev)
   {
    // Limit search to 100 batteries max
    for (int idev = 0; idev < 100; idev++)
     {
      SP_DEVICE_INTERFACE_DATA did = {0};
      did.cbSize = sizeof(did);

      if (SetupDiEnumDeviceInterfaces(hdev,
                                      0,
                                      &GUID_DEVCLASS_BATTERY,
                                      idev,
                                      &did))
       {
        DWORD cbRequired = 0;

        SetupDiGetDeviceInterfaceDetail(hdev,
                                        &did,
                                        0,
                                        0,
                                        &cbRequired,
                                        0);
        if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
         {
          PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd =
            (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR,
                                                         cbRequired);
          if (pdidd)
           {
            pdidd->cbSize = sizeof(*pdidd);
            if (SetupDiGetDeviceInterfaceDetail(hdev,
                                                &did,
                                                pdidd,
                                                cbRequired,
                                                &cbRequired,
                                                0))
             {
              // Enumerated a battery.  Ask it for information.
              HANDLE hBattery = 
                      CreateFile(pdidd->DevicePath,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
              if (INVALID_HANDLE_VALUE != hBattery)
               {
                // Ask the battery for its tag.
                BATTERY_QUERY_INFORMATION bqi = {0};

                DWORD dwWait = 0;
                DWORD dwOut;

                if (DeviceIoControl(hBattery,
                                    IOCTL_BATTERY_QUERY_TAG,
                                    &dwWait,
                                    sizeof(dwWait),
                                    &bqi.BatteryTag,
                                    sizeof(bqi.BatteryTag),
                                    &dwOut,
                                    NULL)
                    && bqi.BatteryTag)
                 {
                  // With the tag, you can query the battery info.
                  BATTERY_INFORMATION bi = {0};
                  bqi.InformationLevel = BatteryInformation;

                  if (DeviceIoControl(hBattery,
                                      IOCTL_BATTERY_QUERY_INFORMATION,
                                      &bqi,
                                      sizeof(bqi),
                                      &bi,
                                      sizeof(bi),
                                      &dwOut,
                                      NULL))
                   {
					  //The manufacturer's suggested capacity, in mWh
					   printf("Battery critical bias is %x, Alert1: 0x%x, Alert 2:0x%x \n", bi.CriticalBias, bi.DefaultAlert1, bi.DefaultAlert2);
                    // Only non-UPS system batteries count
                    if (bi.Capabilities & BATTERY_SYSTEM_BATTERY)
                     {
                      if (!(bi.Capabilities & BATTERY_IS_SHORT_TERM))
                       {
                        dwResult |= GBS_HASBATTERY;
                       }

                      // Query the battery status.
                      BATTERY_WAIT_STATUS bws = {0};
                      bws.BatteryTag = bqi.BatteryTag;

                      BATTERY_STATUS bs;
                      if (DeviceIoControl(hBattery,
                                          IOCTL_BATTERY_QUERY_STATUS,
                                          &bws,
                                          sizeof(bws),
                                          &bs,
                                          sizeof(bs),
                                          &dwOut,
                                          NULL))
                       {

						
                        if (bs.PowerState & BATTERY_POWER_ON_LINE)
                         {
                          dwResult &= ~GBS_ONBATTERY;
                         }
                       }
                     }
                   }
                 }
                CloseHandle(hBattery);
               }
             }
            LocalFree(pdidd);
           }
         }
       }
        else  if (ERROR_NO_MORE_ITEMS == GetLastError())
         {
          break;  // Enumeration failed - perhaps we're out of items
         }
     }
    SetupDiDestroyDeviceInfoList(hdev);
   }

  //  Final cleanup:  If we didn't find a battery, then presume that we
  //  are on AC power.

  if (!(dwResult & GBS_HASBATTERY))
    dwResult &= ~GBS_ONBATTERY;

  return dwResult;
 }


 Dictionary<UInt16, string> StatusCodes;

 private void Form1_Load(object sender, EventArgs e)
 {

	 StatusCodes = new Dictionary<ushort, string>();

	 StatusCodes.Add(1, "The battery is discharging");

	 StatusCodes.Add(2, "The system has access to AC so no battery is being discharged. However, the battery is not necessarily charging");

	 StatusCodes.Add(3, "Fully Charged");

	 StatusCodes.Add(4, "Low");

	 StatusCodes.Add(5, "Critical");

	 StatusCodes.Add(6, "Charging");

	 StatusCodes.Add(7, "Charging and High");

	 StatusCodes.Add(8, "Charging and Low");

	 StatusCodes.Add(9, "Undefined");

	 StatusCodes.Add(10, "Partially Charged");



	 /* Set progress bar values and Properties */

	 progressBar1.Maximum = 100;

	 progressBar1.Style = ProgressBarStyle.Continuous;





	 timer1.Enabled = true;



	 ManagementObjectSearcher mos = new ManagementObjectSearcher("select * from Win32_Battery");

	 foreach(ManagementObject mo in mos.Get())

	 {

		 lblBatteryName.Text = mo["Name"].ToString();

		 UInt16 statuscode = (UInt16)mo["BatteryStatus"];

		 string statusString = StatusCodes[statuscode];

		 lblBatteryStatus.Text = statusString;

	 }

 }



 private void timer1_Tick(object sender, EventArgs e)

 {

	 ManagementObjectSearcher mos = new ManagementObjectSearcher("select * from Win32_Battery where Name='" + lblBatteryName.Text + "'");

	 foreach(ManagementObject mo in mos.Get())

	 {



		 UInt16 statuscode = (UInt16)mo["BatteryStatus"];

		 string statusString = StatusCodes[statuscode];

		 lblBatteryStatus.Text = statusString;



		 /* Set Progress bar according to status  */

		 if (statuscode == 4)

		 {

			 progressBar1.ForeColor = Color.Red;

			 progressBar1.Value = 5;

		 }

		 else if (statuscode == 3)

		 {

			 progressBar1.ForeColor = Color.Blue;

			 progressBar1.Value = 100;

		 }

		 else if (statuscode == 2)

		 {

			 progressBar1.ForeColor = Color.Green;

			 progressBar1.Value = 100;

		 }

		 else if (statuscode == 5)

		 {

			 progressBar1.ForeColor = Color.Red;

			 progressBar1.Value = 1;

		 }

		 else if (statuscode == 6)

		 {

			 progressBar1.ForeColor = Color.Blue;

			 progressBar1.Value = 100;

		 }

	 }

 }






int _tmain(int argc, _TCHAR* argv[])
{
	GetBatteryState();
	return 0;
}


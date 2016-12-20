#include "ueyecamera.h"

using namespace std;

/*Constructor(s)/Destructor(s)*/
UEyeCamera::UEyeCamera()
{
}

UEyeCamera::~UEyeCamera()
{
}

/*****************************************************************************************************/









/*ICamera interface functions*/
void UEyeCamera::Connect()
{
    if(is_InitCamera(&m_CameraHandle, NULL) != IS_SUCCESS)
    {
        UEyeCameraException e(CONNECT_ERROR);
        throw e;
    }
}

void UEyeCamera::Disconnect()
{
    if(is_ExitCamera(m_CameraHandle) != IS_SUCCESS)
    {
        UEyeCameraException e(DISCONNECT_ERROR);
        throw e;
    }
}

void UEyeCamera::UpdateParameters()
{

    m_CamParam.HardwareGain = is_SetHardwareGain (m_CameraHandle, IS_GET_MASTER_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);

    if(is_Exposure(m_CameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE, &m_CamParam.ExposureTime, sizeof(double)) != IS_SUCCESS)
    {
            UEyeCameraException e(GET_PARAMETERS_ERROR);
            throw e;
    }
}

void UEyeCamera::SetParameters()
{
    //Gain is not used in this function yet -> needs to be implemented as soon as it is needed...
    is_SetDisplayMode (m_CameraHandle, IS_SET_DM_DIB);
    is_SetColorMode (m_CameraHandle, IS_CM_RGB8_PACKED);
    is_SetImageSize (m_CameraHandle, m_img_width, m_img_height);
    is_Exposure(m_CameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*) &m_ExposureTime, sizeof(m_ExposureTime));
}


void UEyeCamera::SetLUT(unsigned char *userDefinedLUT, int size)
{
    IS_LUT_STATE lutState;
    bool Set_Enable = false;

    if(is_LUT(m_CameraHandle, IS_LUT_CMD_GET_STATE, (void*) &lutState, sizeof(lutState)) != IS_SUCCESS)
    {
        UEyeCameraException e(SET_LUT_ERROR);
        throw e;
    }

    if(lutState.bLUTEnabled == false)
    {
        IS_LUT_ENABLED_STATE nLutEnabled = IS_LUT_ENABLED;
        if(is_LUT(m_CameraHandle, IS_LUT_CMD_SET_ENABLED, (void*) &nLutEnabled, sizeof(nLutEnabled)) != IS_SUCCESS)
        {
            UEyeCameraException e(SET_LUT_ERROR);
            throw e;
        }else
        {
            Set_Enable = true;
        }
    }else if (lutState.nLUTStateID == IS_LUT_STATE_ID_NOT_SUPPORTED)
    {
        UEyeCameraException e(SET_LUT_ERROR);
        throw e;
    }else
    {
        Set_Enable = true;
    }

    if(Set_Enable == true)
    {
        const int size_out = 64;

        IS_LUT_CONFIGURATION_64 LUT;
        LUT.bAllChannelsAreEqual = true;

        int step = (size/(float)(size_out)+0.5);

        //check step > 0
        int j = 0;
        //int k = 0;
        for(int i=0;i<size_out;i++)
        {

            LUT.dblValues[0][i] = (1.0/255.0) * double (userDefinedLUT[j]);
            j+=step;
            //k += step;
        }

        if(is_LUT(m_CameraHandle, IS_LUT_CMD_SET_USER_LUT, (void*) &LUT, sizeof(LUT)) != IS_SUCCESS)
        {
            UEyeCameraException e(SET_LUT_ERROR);
            throw e;
        }

        wchar_t* pFilename2 = L"/home/stagiairpc/Desktop/lutFile2.xml";
        if(is_LUT(m_CameraHandle, IS_LUT_CMD_SAVE_FILE, (void*) pFilename2 , NULL) != IS_SUCCESS)
        {
            UEyeCameraException e(SET_LUT_ERROR);
            throw e;
        }
    }
}

void UEyeCamera::UpdateLUT(){

    if(is_LUT(m_CameraHandle, IS_LUT_CMD_GET_USER_LUT, (void*) &m_LUT, sizeof(m_LUT)) != IS_SUCCESS)
    {
        UEyeCameraException e(GET_LUT_ERROR);
        throw e;
    }
}

void UEyeCamera::GetCameraID(string &expected_camera_serial,bool &result) //Nog even naar kijken!
{
    result = false;
    INT pnNumCams;
    if(is_GetNumberOfCameras (&pnNumCams) != IS_SUCCESS)
    {
        UEyeCameraException e(CONNECT_ERROR);
        throw e;
    }

    m_CameraList = (UEYE_CAMERA_LIST*) new BYTE [sizeof (DWORD) + pnNumCams * sizeof (UEYE_CAMERA_INFO)];
    m_CameraList->dwCount = pnNumCams;

    //Retrieve camera info
    if (is_GetCameraList(m_CameraList) != IS_SUCCESS)
    {
        UEyeCameraException e(CONNECT_ERROR);
        throw e;
    }

    string test = string(m_CameraList[0].uci[0].SerNo).c_str();

    for(int i=0;i<pnNumCams;i++)
    {
        if(expected_camera_serial.c_str() == test)
        {
            m_CameraID = m_CameraList[i].uci[0].dwCameraID;
            result = true;
        }
    }
}

void UEyeCamera::SingleImageCapture()
{
    if(is_FreezeVideo(m_CameraHandle, IS_WAIT) != IS_SUCCESS)
    {
        UEyeCameraException e(IMAGE_CAPTURE_ERROR);
        throw e;
    }
}

void UEyeCamera::SequentialImageCapture()
{
    if(is_CaptureVideo(m_CameraHandle, IS_DONT_WAIT) != IS_SUCCESS)
    {
        UEyeCameraException e(IMAGE_CAPTURE_ERROR);
        throw e;
    }
}

void UEyeCamera::SetTriggerMode(bool ExternalTriggerMode)
{
    if(ExternalTriggerMode == true)
    {
        is_SetExternalTrigger(m_CameraHandle, IS_SET_TRIGGER_LO_HI); //Sets the hardware trigger on the rising edge
    }else
    {
        is_SetExternalTrigger(m_CameraHandle, IS_SET_TRIGGER_SOFTWARE); //Sets the software trigger to be used. Image will be captured on ImageCapture();
    }
}

void UEyeCamera::AllocateMemory()
{
    SENSORINFO pInfo;
    if(is_GetSensorInfo (m_CameraHandle, &pInfo) != IS_SUCCESS)
    {
        UEyeCameraException e(GET_SENSOR_INFO);
        throw e;
    }
    if(is_AllocImageMem (m_CameraHandle, pInfo.nMaxWidth, pInfo.nMaxHeight, 8, &m_ImageData, &m_MEM_ID) != IS_SUCCESS)
    {
        UEyeCameraException e(ALLOCATE_MEMORY_ERROR);
        throw e;
    }
    if(is_SetImageMem (m_CameraHandle, m_ImageData, m_MEM_ID) != IS_SUCCESS)
    {
        UEyeCameraException e(ALLOCATE_MEMORY_ERROR);
        throw e;
    }
}

void UEyeCamera::AddBufferToSequence()
{
    if(is_AddToSequence (m_CameraHandle, m_ImageData, m_MEM_ID) != IS_SUCCESS)
    {
        UEyeCameraException e(ADD_BUFFER_ERROR);
        throw e;
    }
}

void UEyeCamera::ReleaseMemory()
{
    if(is_FreeImageMem(m_CameraHandle, m_ImageData, m_MEM_ID) != IS_SUCCESS)
    {
        UEyeCameraException e(FREE_IMAGE_MEM_ERROR);
        throw e;
    }
}
/*****************************************************************************************/









/*Get and Set functions*/
IplImage * UEyeCamera::GetIplImage()
{
    return(m_img);
}

char* UEyeCamera::GetImageMemoryPointer()
{
    return (m_ImageData);
}

void UEyeCamera::SetIplImage(int width, int height)
{
    m_img_width = width;
    m_img_height = height;
    m_img=cvCreateImage(cvSize(m_img_width, m_img_height), IPL_DEPTH_8U, 3);
    m_img->ID=0;
    m_img->nChannels=3;
    m_img->alphaChannel=0;
    m_img->depth=8;
    m_img->dataOrder=0;
    m_img->origin=0;
    m_img->align=4;
    m_img->width=m_img_width;
    m_img->height=m_img_height;
    m_img->roi=NULL;
    m_img->maskROI=NULL;
    m_img->imageId=NULL;
    m_img->tileInfo=NULL;
    m_img->imageSize=3*m_img_width*m_img_height;
    m_img->widthStep=3*m_img_width;
}

void UEyeCamera::SetCameraHandle()
{
    m_CameraHandle = m_CameraID;
}

void UEyeCamera::GetCameraHandle(int &CameraHandle)
{
    CameraHandle = m_CameraHandle;
}

void UEyeCamera::GetImageParam(SENSORINFO& sensorInfo)
{
    if(is_GetSensorInfo (m_CameraHandle, &sensorInfo) != IS_SUCCESS)
    {
        UEyeCameraException e(GET_SENSOR_INFO);
        throw e;
    }
}

void UEyeCamera::GetCameraParameters(CameraParameters &camParameters)
{
    camParameters.ExposureTime = m_CamParam.ExposureTime;
    camParameters.HardwareGain = m_CamParam.HardwareGain;
}

void UEyeCamera::SetCameraParameters(int gain, double exposure)
{
    m_CamParam.ExposureTime = exposure;
    m_CamParam.HardwareGain = gain;
}

void UEyeCamera::GetImageMemoryID(int &MemoryID)
{
    MemoryID = m_MEM_ID;
}

void UEyeCamera::SetCameraBuffer(int BitsPerPixel)
{
    m_img_bpp = BitsPerPixel;
    INT rnet = is_AllocImageMem(m_CameraHandle, m_img_width, m_img_height, m_img_bpp, &m_ImageData, &m_MEM_ID);
    INT rnet2 = is_SetImageMem (m_CameraHandle, m_ImageData, m_MEM_ID);
}

/***************************************************************************************************/









/*UEye-only functions*/
void UEyeCamera::SaveImage(string FILEPATH, string ImageFormat, int ImageQuality)
{
    IMAGE_FILE_PARAMS ImageFileParams;

    if(ImageFormat == "JPEG")
    {
        ImageFileParams.nFileType = IS_IMG_JPG;
    }else if(ImageFormat == "PNG")
    {
        ImageFileParams.nFileType = IS_IMG_PNG;
    }else if(ImageFormat == "BMP")
    {
        ImageFileParams.nFileType = IS_IMG_BMP;
    }else
    {
        UEyeCameraException e(IMAGE_FORMAT_INPUT_ERROR);
        throw e;
    }

    string FilePath = FILEPATH;
    std::wstring widestr = std::wstring(FilePath.begin(), FilePath.end());
    wchar_t* pwchFileName = const_cast<wchar_t*>(widestr.c_str());

    UINT ImageID = UINT(m_MEM_ID);
    ImageFileParams.pwchFileName = pwchFileName;
    ImageFileParams.pnImageID = &ImageID;
    ImageFileParams.ppcImageMem = &m_ImageData;
    ImageFileParams.nQuality = ImageQuality;

    unsigned char f = 15;
    cout << f << endl;
    unsigned char a = m_ImageData[0];
    unsigned char b = m_ImageData[1];
    unsigned char c = m_ImageData[2];
    unsigned char d = m_ImageData[3];

    int e = ((int*)m_ImageData)[0];


    if(is_ImageFile(m_CameraHandle, IS_IMAGE_FILE_CMD_SAVE, &ImageFileParams, sizeof(ImageFileParams)) != IS_SUCCESS)
    {
        UEyeCameraException e(SAVE_IMAGE_ERROR);
        throw e;
    }
}

void UEyeCamera::GetConnectedCameras()
{
    INT pnNumCams;
    if(is_GetNumberOfCameras (&pnNumCams) != IS_SUCCESS)
    {
        throw "Getting the No of cameras failed";
    }else
    {
        if( pnNumCams >= 1 )
        {
          // Create new list with suitable size
          m_CameraList = (UEYE_CAMERA_LIST*) new BYTE [sizeof (DWORD) + pnNumCams * sizeof (UEYE_CAMERA_INFO)];
          m_CameraList->dwCount = pnNumCams;

          //Retrieve camera info
          if (is_GetCameraList(m_CameraList) != IS_SUCCESS)
          {
              UEyeCameraException e(CONNECT_ERROR);
              throw e;
          }
        }else
        {
            UEyeCameraException e(CONNECT_ERROR);
            throw e;
        }
    }
}



void UEyeCamera::GetUEyeCameraTriggerInputStatus(int &TriggerStatus)
{
    TriggerStatus = is_SetExternalTrigger(m_CameraHandle, IS_GET_TRIGGER_STATUS);
}

void UEyeCamera::GetUEyeCameraAmountOfTriggers(int &ammountOfTriggers)
{
    if(is_SetTriggerCounter (m_CameraHandle, ammountOfTriggers) != IS_SUCCESS)
    {
        UEyeCameraException e(GET_TRIGGER_AMMOUNT_ERROR);
        throw e;
    }
}

void UEyeCamera::EnableEvent(INT EventID)
{
    if(is_EnableEvent ( m_CameraHandle, EventID) != IS_SUCCESS)
    {
        UEyeCameraException e(ENABLE_EVENT_ERROR);
        throw e;
    }
}

void UEyeCamera::WaitOnEvent(INT EventID, INT TimeOut)
{
    if(TimeOut == -1)
    {
        if(is_WaitEvent (m_CameraHandle, EventID, INFINITE) != IS_SUCCESS)
        {
            UEyeCameraException e(ENABLE_EVENT_ERROR);
            throw e;
        }
    }else
    {
        if(is_WaitEvent (m_CameraHandle, EventID, TimeOut) != IS_SUCCESS)
        {
            UEyeCameraException e(ENABLE_EVENT_ERROR);
            throw e;
        }
    }

}

void UEyeCamera::ConvertImageFromBufferToIplImage()
{
    void *pMemVoid; //pointer to where the image is stored
    is_GetImageMem (m_CameraHandle, &pMemVoid);
    m_img->imageData=(char*)pMemVoid;  //the pointer to imagaData
    m_img->imageDataOrigin=(char*)pMemVoid; //and again
}



void UEyeCamera::SaveIplImageList(int ListSize, QList<IplImage *> ImageList, string SavePath)
{
    char szFullPath[256];
    for(int i = 0; i < ListSize; i++)
    {
        sprintf(szFullPath, "%s%4.4d.bmp", SavePath.c_str(), i);
        cvSaveImage(szFullPath, ImageList[i]);
    }
}

/*******************************************************************************************************/

























/*****OBSOLETE FUNCTIONS*****/

/*Function GetCameraID makes this function obsolete*/
void UEyeCamera::GetAvailableCameraIdentifiers(vector<UEYE_CAMERA_INFO> &camera_identifier_list)
{

    camera_identifier_list.clear();
    INT pnNumCams;
    if(is_GetNumberOfCameras (&pnNumCams) != IS_SUCCESS)
    {
        UEyeCameraException e(CONNECT_ERROR);
        throw e;
    }

    m_CameraList = (UEYE_CAMERA_LIST*) new BYTE [sizeof (DWORD) + pnNumCams * sizeof (UEYE_CAMERA_INFO)];
    m_CameraList->dwCount = pnNumCams;

    //Retrieve camera info
    if (is_GetCameraList(m_CameraList) != IS_SUCCESS)
    {
        UEyeCameraException e(CONNECT_ERROR);
        throw e;
    }

    UEYE_CAMERA_INFO element;
    for(int i=0;i<pnNumCams;i++)
    {
        element = m_CameraList[i].uci[0];
        camera_identifier_list.push_back(element);
    }
}

/************************OBSOLETE FUNCTION******************************
void UEyeCamera::SetParameters(CAM_REGISTER reg, CAM_VALUE value)
{
    if(reg == 0x00)
    {
        if(value.i_val < 0 || value.i_val > 100){
            throw "Not a valid gain value!";
        }else
        {
            if(is_SetHardwareGain (m_CameraHandle, value.i_val, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) != IS_SUCCESS)
            {
                UEyeCameraException e(SET_PARAMETERS_ERROR);
                throw e;
            }
        }
    }
    if(reg == 0x01)
    {
        double *time_ms = &value.d_value;
        if(is_Exposure(m_CameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, time_ms, sizeof(double)) != IS_SUCCESS)
        {
            UEyeCameraException e(SET_PARAMETERS_ERROR);
            throw e;
        }
    }
}
************************************************************************/

/*
void UEyeCamera::SetParameters()
{
    if(m_CamParam.HardwareGain > 0 && m_CamParam.HardwareGain < 100)
    {
        INT rnet = is_SetHardwareGain (m_CameraHandle, m_CamParam.HardwareGain, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
        /*
        if(is_SetHardwareGain (m_CameraHandle, m_CameraParameters.i_val, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) != IS_SUCCESS)
        {
            UEyeCameraException e(SET_PARAMETERS_ERROR);
            throw e;
        }
    }

    if(m_CamParam.ExposureTime > 0.0 && m_CamParam.ExposureTime < 11.45)
    {
        double *time_ms = &m_CamParam.ExposureTime;
        //INT rnet2 = is_Exposure(m_CameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, time_ms, sizeof(double));
        if(is_Exposure(m_CameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, time_ms, sizeof(double)) != IS_SUCCESS)
        {
          UEyeCameraException e(SET_PARAMETERS_ERROR);
            throw e;
        }
    }
}*/






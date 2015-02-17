
// AudioProcessDoc.cpp : implementation of the CAudioProcessDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "AudioProcess.h"
#endif

#include "AudioProcessDoc.h"
#include "ProcessDlg.h"

#include <vector>
#include <fstream>

#include <propkey.h>

using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CAudioProcessDoc::OnProcessCopy()
{
   // Call to open the processing output
   if(!ProcessBegin())
      return;

   short audio[2];

   for(int i=0;  i<SampleFrames();  i++)
   {                 
      ProcessReadFrame(audio);

      audio[0] = short(audio[0] * m_amplitude);
      audio[1] = short(audio[1] * m_amplitude);

      ProcessWriteFrame(audio);

      // The progress control
      if(!ProcessProgress(double(i) / SampleFrames()))
         break;
   }

   
   // Call to close the generator output
   ProcessEnd();
}



// CAudioProcessDoc

IMPLEMENT_DYNCREATE(CAudioProcessDoc, CDocument)

BEGIN_MESSAGE_MAP(CAudioProcessDoc, CDocument)
	ON_COMMAND(ID_PROCESS_FILEOUTPUT, &CAudioProcessDoc::OnProcessFileoutput)
	ON_COMMAND(ID_PROCESS_AUDIOOUTPUT, &CAudioProcessDoc::OnProcessAudiooutput)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_FILEOUTPUT, &CAudioProcessDoc::OnUpdateProcessFileoutput)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_AUDIOOUTPUT, &CAudioProcessDoc::OnUpdateProcessAudiooutput)
	ON_COMMAND(ID_PROCESS_COPY, &CAudioProcessDoc::OnProcessCopy)
	ON_COMMAND(ID_PROCESS_PARAMETERS, &CAudioProcessDoc::OnProcessParameters)
	ON_COMMAND(ID_PRACTICE_B1, &CAudioProcessDoc::OnPracticeB1)
	ON_COMMAND(ID_PRACTICE_B2, &CAudioProcessDoc::OnPracticeB2)
	ON_COMMAND(ID_PRACTICE_C3, &CAudioProcessDoc::OnPracticeC3)
	ON_COMMAND(ID_PRACTICE_B5, &CAudioProcessDoc::OnPracticeB4)
END_MESSAGE_MAP()




// CAudioProcessDoc construction/destruction

CAudioProcessDoc::CAudioProcessDoc()
{
   m_audiooutput = true;
   m_fileoutput = false;

   m_numChannels = 2;
   m_sampleRate = 44100.;
   m_numSampleFrames = 0;
   m_amplitude = 1.0;
}

CAudioProcessDoc::~CAudioProcessDoc()
{
}

BOOL CAudioProcessDoc::OnNewDocument()
{
	return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CAudioProcessDoc serialization

void CAudioProcessDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CAudioProcessDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CAudioProcessDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CAudioProcessDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CAudioProcessDoc diagnostics

#ifdef _DEBUG
void CAudioProcessDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CAudioProcessDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CAudioProcessDoc commands


BOOL CAudioProcessDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	
	if(!m_wavein.Open(lpszPathName))
		return FALSE;

	m_sampleRate = m_wavein.SampleRate();
	m_numChannels = m_wavein.NumChannels();
	m_numSampleFrames = m_wavein.NumSampleFrames();
   
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// The following functions manage the audio processing process, 
// directing output to the waveform buffer, file, and/or audio 
// output.  
//
/////////////////////////////////////////////////////////////////////////////


//
// Name :        CAudioProcessDoc::ProcessBegin()
// Description : This function starts the audio processing process.
//               It opens the waveform storage, opens the file
//               if file output is requested, and opens the 
//               audio output if audio output is requested.
//               Be sure to call EndProcess() when done.
// Returns :     true if successful...
//

bool CAudioProcessDoc::ProcessBegin()
{
    m_wavein.Rewind();

	// 
	// Waveform storage
	//

	m_waveformBuffer.Start(NumChannels(), SampleRate());

    if(m_fileoutput)
    {
      if(!OpenProcessFile(m_waveout))
         return false;
    }

   ProgressBegin(this);

   if(m_audiooutput)
   {
      m_soundstream.SetChannels(NumChannels());
      m_soundstream.SetSampleRate(int(SampleRate()));

      m_soundstream.Open(((CAudioProcessApp *)AfxGetApp())->DirSound());
   }

   return true;
}


//
// Name :        CAudioProcessDoc::ProcessReadFrame()
// Description : Read a frame of input from the current audio file.
//

void CAudioProcessDoc::ProcessReadFrame(short *p_frame)
{
   m_wavein.ReadFrame(p_frame);
}


//
// Name :        CAudioProcessDoc::ProcessWriteFrame()
// Description : Write a frame of output to the current generation device.
//

void CAudioProcessDoc::ProcessWriteFrame(short *p_frame)
{
    m_waveformBuffer.Frame(p_frame);

   if(m_fileoutput)
      m_waveout.WriteFrame(p_frame);

   if(m_audiooutput)
      m_soundstream.WriteFrame(p_frame);
}


//
// Name :        CAudioProcessDoc::ProcessEnd()
// Description : End the generation process.
//

void CAudioProcessDoc::ProcessEnd()
{
    m_waveformBuffer.End();

   if(m_fileoutput)
      m_waveout.close();

   if(m_audiooutput)
      m_soundstream.Close();

   ProgressEnd(this);


}

//
// Name :        CAudioProcessDoc::OpenProcessFile()
// Description : This function opens the audio file for output.
// Returns :     true if successful...
//

bool CAudioProcessDoc::OpenProcessFile(CWaveOut &p_wave)
{
   p_wave.NumChannels(m_numChannels);
   p_wave.SampleRate(m_sampleRate);

	static WCHAR BASED_CODE szFilter[] = L"Wave Files (*.wav)|*.wav|All Files (*.*)|*.*||";

	CFileDialog dlg(FALSE, L".wav", NULL, 0, szFilter, NULL);
	if(dlg.DoModal() != IDOK)
      return false;

    p_wave.open(dlg.GetPathName());
   if(p_wave.fail())
      return false;

   return true;
}



void CAudioProcessDoc::OnProcessFileoutput()
{
	m_fileoutput = !m_fileoutput;
}


void CAudioProcessDoc::OnProcessAudiooutput()
{
   m_audiooutput = !m_audiooutput;
}


void CAudioProcessDoc::OnUpdateProcessFileoutput(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(m_fileoutput);	
}


void CAudioProcessDoc::OnUpdateProcessAudiooutput(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(m_audiooutput);	
}



void CAudioProcessDoc::OnProcessParameters()
{
   CProcessDlg dlg;
   
   dlg.m_amplitude = m_amplitude;

   if(dlg.DoModal() == IDOK)
   {
      m_amplitude = dlg.m_amplitude;
   }
}

/**
*	FLANGE

	Implement a flange effect.  The effect will consist of the sound added to a delayed version of the sound.  
	The amount of delay will vary from 2 to 10 milliseconds at a sinusoidal rate of 0.25Hz.  
	This means that at time 0, the delay will be 6ms, the delay will increase to 10 milliseconds over one second, 
		decrease to 2 milliseconds in two seconds, then increase back to 6ms over the next second.  
	The delay amount will be 0.006 + sin(0.25 * 2 * PI * t) * 0.004 seconds.
*/
void CAudioProcessDoc::OnPracticeB1()
{
	// Call to open the processing output
	if (!ProcessBegin())
		return;

	short audio[2];

	const int QUEUESIZE = 200000;
	const double DELAY = 1.0;

	std::vector<short> queue;
	queue.resize(QUEUESIZE);

	int wrloc = 0;

	double time = 0;

	for (int i = 0; i<SampleFrames(); i++, time += 1. / SampleRate())
	{
		ProcessReadFrame(audio);

		wrloc = (wrloc + 2) % QUEUESIZE;
		queue[wrloc] = audio[0];
		queue[wrloc + 1] = audio[1];

		double flange = double(0.006 + sin(0.25 * 2 * M_PI * time) * 0.004);
		int delaylength = int((flange * SampleRate() + 0.5)) * 2;
		int rdloc = (wrloc + QUEUESIZE - delaylength) % QUEUESIZE;

		audio[0] = audio[0] / 2 + queue[rdloc++] / 2;
		audio[1] = audio[1] / 2 + queue[rdloc] / 2;

		ProcessWriteFrame(audio);

		// The progress control
		if (!ProcessProgress(double(i) / SampleFrames()))
			break;
	}


	// Call to close the generator output
	ProcessEnd();
}

/**
* CHORUS

	Implement a chorus effect.  The effect will consist of the sound added to a delayed version of the sound.  
	The amount of delay will vary from 20 to 30 milliseconds at a sinusoidal rate of 0.25Hz.  
	This means that at time 0, the delay will be 25ms, the delay will increase to 30 milliseconds over one second, 
		decrease to 20 milliseconds in two seconds, then increase back to 25ms over the next second.  
	The delay amount will be 0.025 + sin(0.25 * 2 * PI * t) * 0.005 seconds.
*/
void CAudioProcessDoc::OnPracticeB2()
{
	// Call to open the processing output
	if (!ProcessBegin())
		return;

	short audio[2];

	const int QUEUESIZE = 200000;
	const double DELAY = 1.0;

	std::vector<short> queue;
	queue.resize(QUEUESIZE);

	int wrloc = 0;

	double time = 0;

	for (int i = 0; i<SampleFrames(); i++, time += 1. / SampleRate())
	{
		ProcessReadFrame(audio);

		wrloc = (wrloc + 2) % QUEUESIZE;
		queue[wrloc] = audio[0];
		queue[wrloc + 1] = audio[1];

		double chorus = double(0.025 + sin(0.25 * 2 * M_PI * time) * 0.005);
		int delaylength = int((chorus * SampleRate() + 0.5)) * 2;
		int rdloc = (wrloc + QUEUESIZE - delaylength) % QUEUESIZE;

		audio[0] = audio[0] / 2 + queue[rdloc++] / 2;
		audio[1] = audio[1] / 2 + queue[rdloc] / 2;

		ProcessWriteFrame(audio);

		// The progress control
		if (!ProcessProgress(double(i) / SampleFrames()))
			break;
	}


	// Call to close the generator output
	ProcessEnd();
}

/**
* DOPPLER EFFECT

	Create an effect that simulates the Doppler effect. 
	he audio should be played a little bit fast at the beginning and a little slow at the end, like the sound of a siren changing pitch as it passes you. 
	The exact pitch you should play at any point is in time is: p = 1 + 0.1cos(PI(t/d)), where t is the current time, d is the duration of the audio, 
		and p is the relative pitch. 
	For example, suppose I have a 10 second audio clip. At time 1, the audio should play p = 1 + 0.1(cos(PI(1/10)) = 1.0951. 
	This means the audio is playing 9.51% fast. You may load the audio into an array if you wish for this problem.
*/
void CAudioProcessDoc::OnPracticeC3()
{
	// Call to open the processing output
	if (!ProcessBegin())
		return;

	short audio[2];

	double relativePitch = 0;
	double currentSample = 0;
	double time = 0;
	
	vector<short> wavetable;


	// Reading and adding the audio to our wavetable
	for (int x = 0; x < SampleFrames(); x++){
		ProcessReadFrame(audio);
		wavetable.push_back(audio[0]);
	}


	int i = 0;
	// While there are frames left
	while (i < SampleFrames())
	{
		// Calculate our audio duration
		double duration = SampleFrames() / SampleRate();
		
		// Calculate the relative pitch
		relativePitch = (1 + ((0.1) * cos(M_PI * (time / duration))));

		// Add the relative pitch to our sample
		currentSample += relativePitch;

		// Pull audio from wavetable
		audio[0] = wavetable[i];


		ProcessWriteFrame(audio);


		// Calculate new index (+ .5 for rounding)
		i = int(currentSample + 0.5);

		// Increment time
		time += 1. / SampleRate();


		// The progress control
		if (!ProcessProgress(double(i) / SampleFrames()))
			break;
	}

	// Call to close the generator output
	ProcessEnd();
}


/**
* PROCESS VIBRATO 

	Create a 3Hz vibrato that varies the pitch of the audio in audioB.wav up,
		and down by exactly 10%.
	You may load the audio into an array if you wish for this problem.
*/
void CAudioProcessDoc::OnPracticeB4()
{
	// Call to open the processing output
	if (!ProcessBegin())
		return;

	short audio[2];

	double currentSample = 0;
	double time = 0;

	double radiansVibrato = 0;
	double vibratoRate = 3;				
	double vibrato = .1;				/// 10% of

	vector<short> wavetable;


	// Reading and adding the audio to our wavetable
	for (int x = 0; x < SampleFrames(); x++){
		ProcessReadFrame(audio);
		wavetable.push_back(audio[0]);
	}


	int i = 0;
	// While there are frames left
	while (i < SampleFrames())
	{
		// Pull audio from wavetable
		audio[0] = wavetable[i];

		// Calculate our vibrato (should be from -.1 to .1), with frequency of 3Hz (goes inside of sine)
		//		- To oscillate between numbers, multiply by sin
		//		- To increase frequency of osciallation (cycle), multiple rate in the sin
		radiansVibrato = (vibrato * sin(2 * M_PI * time * vibratoRate));

		// Add the diff to our sample (adding 1 so we can change samples - otherwise, repeats 0)
		currentSample += 1 + radiansVibrato;


		ProcessWriteFrame(audio);


		// Calculate new index (+ .5 for rounding)
		i = int(currentSample + 0.5);

		// Increment time
		time += 1. / SampleRate();


		// The progress control
		if (!ProcessProgress(double(i) / SampleFrames()))
			break;
	}

	// Call to close the generator output
	ProcessEnd();
}


// AudioGenerateDoc.cpp : implementation of the CAudioGenerateDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "AudioProcess.h"
#endif

#include "AudioGenerateDoc.h"
#include "GenerateDlg.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAudioGenerateDoc

IMPLEMENT_DYNCREATE(CAudioGenerateDoc, CDocument)

BEGIN_MESSAGE_MAP(CAudioGenerateDoc, CDocument)
	ON_COMMAND(ID_GENERATE_FILEOUTPUT, &CAudioGenerateDoc::OnGenerateFileoutput)
	ON_COMMAND(ID_GENERATE_AUDIOOUTPUT, &CAudioGenerateDoc::OnGenerateAudiooutput)
	ON_COMMAND(ID_GENERATE_SINEWAVE, &CAudioGenerateDoc::OnGenerateSinewave)
	ON_COMMAND(ID_GEN_PARAMETERS, &CAudioGenerateDoc::OnGenParameters)
	ON_UPDATE_COMMAND_UI(ID_GENERATE_FILEOUTPUT, &CAudioGenerateDoc::OnUpdateGenerateFileoutput)
	ON_UPDATE_COMMAND_UI(ID_GENERATE_AUDIOOUTPUT, &CAudioGenerateDoc::OnUpdateGenerateAudiooutput)
	ON_COMMAND(ID_PRACTICE_A1, &CAudioGenerateDoc::OnPracticeA1)
	ON_COMMAND(ID_PRACTICE_A2, &CAudioGenerateDoc::OnPracticeA2)
	ON_COMMAND(ID_PRACTICE_A3, &CAudioGenerateDoc::OnPracticeA3)
	ON_COMMAND(ID_PRACTICE_A4, &CAudioGenerateDoc::OnPracticeA4)
	ON_COMMAND(ID_PRACTICE_A5, &CAudioGenerateDoc::OnPracticeA5)
	ON_COMMAND(ID_PRACTICE_A6, &CAudioGenerateDoc::OnPracticeA6)
	ON_COMMAND(ID_PRACTICE_A7, &CAudioGenerateDoc::OnPracticeA7)
	ON_COMMAND(ID_PRACTICE_A8, &CAudioGenerateDoc::OnPracticeA8)
END_MESSAGE_MAP()


/*! Example procedure that generates a sine wave.
 * 
 * The sine wave frequency is set by m_freq1
 */
void CAudioGenerateDoc::OnGenerateSinewave()
{
	// Call to open the generator output
	if(!GenerateBegin())
		return;

	short audio[2];

	for(double time=0.;  time < m_duration;  time += 1. / m_sampleRate)
	{                 
		audio[0] = short(m_amplitude * sin(time * 2 * M_PI * m_freq1));
		audio[1] = short(m_amplitude * sin(time * 2 * M_PI * m_freq1));

		GenerateWriteFrame(audio);

		// The progress control
		if(!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();
}




// CAudioGenerateDoc construction/destruction

CAudioGenerateDoc::CAudioGenerateDoc()
{
    m_freq1 = 440.;
    m_freq2 = 440.;
    m_vibratoamp = 20.;

    m_audiooutput = true;
    m_fileoutput = false;

    m_numChannels = 2;
    m_sampleRate = 44100.;
    m_duration = 10.0;
    m_amplitude = 3276.7;
}

CAudioGenerateDoc::~CAudioGenerateDoc()
{
}

BOOL CAudioGenerateDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}



void CAudioGenerateDoc::OnGenParameters()
{
   CGenerateDlg dlg;

   // Initialize dialog data
   dlg.m_Channels = m_numChannels;
   dlg.m_SampleRate = m_sampleRate;
   dlg.m_Duration = m_duration;
   dlg.m_Amplitude = m_amplitude;
   dlg.m_freq1 = m_freq1;
   dlg.m_freq2 = m_freq2;
   dlg.m_vibratoamp = m_vibratoamp;

   // Invoke the dialog box
   if(dlg.DoModal() == IDOK)
   {
      // Accept the values
      m_numChannels = dlg.m_Channels;
      m_sampleRate = dlg.m_SampleRate;
      m_duration = dlg.m_Duration;
      m_amplitude = dlg.m_Amplitude;
      m_freq1 = dlg.m_freq1;
      m_freq2 = dlg.m_freq2;
      m_vibratoamp = dlg.m_vibratoamp;

      UpdateAllViews(NULL);  
   }
}


#ifdef SHARED_HANDLERS

// Support for thumbnails
void CAudioGenerateDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
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
void CAudioGenerateDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CAudioGenerateDoc::SetSearchContent(const CString& value)
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

// CAudioGenerateDoc diagnostics

#ifdef _DEBUG
void CAudioGenerateDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CAudioGenerateDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CAudioGenerateDoc commands






/////////////////////////////////////////////////////////////////////////////
//
// The following functions manage the audio generation process, 
// directing output to the waveform buffer, file, and/or audio 
// output.  
//
/////////////////////////////////////////////////////////////////////////////


//
// Name :        CAudioGenerateDoc::GenerateBegin()
// Description : This function starts the audio generation process.
//               It opens the waveform storage, opens the file
//               if file output is requested, and opens the 
//               audio output if audio output is requested.
//               Be sure to call EndGenerate() when done.
// Returns :     true if successful...
//

bool CAudioGenerateDoc::GenerateBegin()
{
	// 
	// Waveform storage
	//

	m_waveformBuffer.Start(NumChannels(), SampleRate());

	if(m_fileoutput)
	{
	  if(!OpenGenerateFile(m_wave))
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
// Name :        CAudioGenerateDoc::GenerateWriteFrame()
// Description : Write a frame of output to the current generation device.
// Parameters :  p_frame - An arrays with the number of channels in samples
//               in it.
//

void CAudioGenerateDoc::GenerateWriteFrame(short *p_frame)
{
    m_waveformBuffer.Frame(p_frame);

    if(m_fileoutput)
        m_wave.WriteFrame(p_frame);

    if(m_audiooutput)
        m_soundstream.WriteFrame(p_frame);
}


//
// Name :        CAudioGenerateDoc::GenerateEnd()
// Description : End the generation process.
//

void CAudioGenerateDoc::GenerateEnd()
{
    m_waveformBuffer.End();

    if(m_fileoutput)
        m_wave.close();

    if(m_audiooutput)
        m_soundstream.Close();

    ProgressEnd(this);
}

//
// Name :        CAudioGenerateDoc::OpenGenerateFile()
// Description : This function opens the audio file for output.
// Returns :     true if successful...
//

bool CAudioGenerateDoc::OpenGenerateFile(CWaveOut &p_wave)
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



void CAudioGenerateDoc::OnGenerateFileoutput()
{
   m_fileoutput = !m_fileoutput;
}


void CAudioGenerateDoc::OnGenerateAudiooutput()
{
   m_audiooutput = !m_audiooutput;
}


void CAudioGenerateDoc::OnUpdateGenerateFileoutput(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(m_fileoutput);	
}


void CAudioGenerateDoc::OnUpdateGenerateAudiooutput(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(m_audiooutput);	
}


/**
* Transition from a sine to a sawtooth over 5 seconds (all harmonic frequencies up to nyquist)
*/
void CAudioGenerateDoc::OnPracticeA1()
{
	// Call to open the generator output
	if (!GenerateBegin())
		return;
	

	double timePeriod = 5.0;									// Seconds
	double startingFrequency = 622.25;							// Hz
	double nyquistFrequency = m_sampleRate / 2;					// 22,050 Hz

	int maxHarmonic = (((m_sampleRate / 2) -					// Maximum harmonic we will add
		startingFrequency) / startingFrequency) + 1;
	int numHarmonics = ((m_sampleRate / 2) -					// How many harmonics we need to add
		startingFrequency) / startingFrequency;
	int harmonicCounter = 2;									// Which harmonic are we adding (no such thing as first - start at 2nd)
	double harmonicRate = numHarmonics / timePeriod;			// Harmonics per second
	double inverseHarmonicRate = 1 / harmonicRate;				// Each X seconds, we add a harmonic (.147 in this case)


	short audio[2];												// Audio channels

	// For each frame - while keeping time
	for (double time = 0.; time < m_duration; time += 1. / m_sampleRate)
	{
		// Fundamental frequency
		short sample = short((m_amplitude * sin(time * 2 * M_PI * startingFrequency)));

		// Add all old harmonics
		for (int i = 2; i <= harmonicCounter; i++) {
			// Add the harmonic
			sample += short((m_amplitude / i) * (sin(time * 2 * M_PI * i * startingFrequency)));
		}

		/* If it is time to add a new harmonic and we can add another harmonic without breaking Nyquist barrier
				- If we have 4 harmonics to add (say for if we have a 200Hz signal, and are going to 1000Hz), then we should add harmonics 2, 3, 4, and 5
					- Thus, the upper limit is the number of harmonics, plus 1
		*/
		if (time > (harmonicCounter * inverseHarmonicRate) && harmonicCounter < (numHarmonics + 1)) {
			// Moving to ze next harmonic!
			harmonicCounter += 1;
		}

		audio[0] = sample;
		audio[1] = sample;

		GenerateWriteFrame(audio);

		// The progress control
		if (!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();
}

/**
* Transition from a sine to a square over 5 seconds (all odd harmonic frequencies up to nyquist)
*/
void CAudioGenerateDoc::OnPracticeA2()
{
	// Call to open the generator output
	if (!GenerateBegin())
		return;


	double timePeriod = 5.0;									// Seconds
	double startingFrequency = 622.25;							// Hz
	double nyquistFrequency = m_sampleRate / 2;					// 22,050 Hz

	int maxHarmonic = (((m_sampleRate / 2) -					// Maximum harmonic we will add
		startingFrequency) / startingFrequency) + 1;
	int numHarmonics = (((m_sampleRate / 2) -					// How many harmonics we need to add
		startingFrequency) / startingFrequency) / 2;				// Over 2, because we only need odds
	int harmonicCounter = 3;									// Which harmonic are we adding (no such thing as first - start at 2nd)
	double harmonicRate = numHarmonics / timePeriod;			// Harmonics per second
	double inverseHarmonicRate = 1 / harmonicRate;				// Each X seconds, we add a harmonic (.147 in this case)


	short audio[2];												// Audio channels

	// For each frame - while keeping time
	for (double time = 0.; time < m_duration; time += 1. / m_sampleRate)
	{
		// Fundamental frequency
		short sample = short((m_amplitude * sin(time * 2 * M_PI * startingFrequency)));

		// Add all old harmonics
		for (int i = 3; i <= harmonicCounter; i += 2) {
			// Add the harmonic
			sample += short((m_amplitude / i) * (sin(time * 2 * M_PI * i * startingFrequency)));
		}

		// If it is time to add a new harmonic and we can add another harmonic without breaking Nyquist barrier
		if (time > ((harmonicCounter / 2) * inverseHarmonicRate) && (harmonicCounter + 2) < maxHarmonic) {
			// Moving to ze next harmonic!
			harmonicCounter += 2;
		}

		audio[0] = sample;
		audio[1] = sample;

		GenerateWriteFrame(audio);

		// The progress control
		if (!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();
}

/**
* Transition from a sine to a triangle over 5 seconds (all odd harmonic frequencies up to nyquist, every other one is negative (3 = negative, 7 = negative...))
*/
void CAudioGenerateDoc::OnPracticeA3()
{
	// Call to open the generator output
	if (!GenerateBegin())
		return;


	double timePeriod = 5.0;									// Seconds
	double startingFrequency = 622.25;							// Hz
	double nyquistFrequency = m_sampleRate / 2;					// 22,050 Hz

	int maxHarmonic = (((m_sampleRate / 2) -					// Maximum harmonic we will add
		startingFrequency) / startingFrequency) + 1;
	int numHarmonics = (((m_sampleRate / 2) -					// How many harmonics we need to add
		startingFrequency) / startingFrequency) / 2;				// Over 2, because we only need odds
	int harmonicCounter = 3;									// Which harmonic are we adding (no such thing as first - start at 2nd)
	double harmonicRate = numHarmonics / timePeriod;			// Harmonics per second
	double inverseHarmonicRate = 1 / harmonicRate;				// Each X seconds, we add a harmonic (.147 in this case)

	int negativeFactor = -1;									// Flips every other harmonic for triangle wave


	short audio[2];												// Audio channels

	// For each frame - while keeping time
	for (double time = 0.; time < m_duration; time += 1. / m_sampleRate)
	{
		// Fundamental frequency
		short sample = short((m_amplitude * sin(time * 2 * M_PI * startingFrequency)));

		// Add all old harmonics
		for (int i = 3; i <= harmonicCounter; i += 2) {
			// Add the harmonic
			sample += short((m_amplitude / (i * i)) * (sin(time * 2 * M_PI * i * startingFrequency)) * negativeFactor);

			negativeFactor *= -1;
		}

		// If it is time to add a new harmonic and we can add another harmonic without breaking Nyquist barrier
		if (time > ((harmonicCounter / 2) * inverseHarmonicRate) && (harmonicCounter + 2) < maxHarmonic) {
			// Moving to ze next harmonic!
			harmonicCounter += 2;
		}

		audio[0] = sample;
		audio[1] = sample;

		GenerateWriteFrame(audio);

		// The progress control
		if (!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();
}



//
//// Add harmonic (amplitude over time, and wave multiplied by harmonic number (time as well))
//sample += short((m_amplitude / harmonicCounter) * (sin(time * 2 * M_PI * harmonicCounter * startingFrequency)) + (sin(time * 2 * M_PI * harmonicCounter * startingFrequency)));

/**
* Sweeps from 100Hz to 4000Hz in 5 secs, then back in 5 seconds
*/
void CAudioGenerateDoc::OnPracticeA4()
{
	// Call to open the generator output
	if (!GenerateBegin())
		return;

	double startingFrequency = 100;								// Hz
	double endingFrequency = 4000;
	double radians = 0;

	short audio[2];												// Audio channels

	// For each frame - while keeping time
	for (double time = 0.; time < m_duration; time += 1. / m_sampleRate)
	{
		double freq = startingFrequency;

		if (time < 5)
			freq += (time / 5) * endingFrequency;
		else if (time >= 5)
			freq += (1 - ((time - 5) / 5)) * endingFrequency;

		//short sample = short((m_amplitude * sin(time * 2 * M_PI * freq)));

		short sample = short(m_amplitude * sin(radians));
		audio[0] = audio[1] = sample;

		// Increment phases
		radians += (2 * M_PI * freq) / m_sampleRate;

		GenerateWriteFrame(audio);

		// The progress control
		if (!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();
}

/**
* Creates a 554.36Hz sine wave, at amplitude 3000, and a 10Hz vibrato. Over first five seconds
*		the vibrato RATE goes from .1 to 3Hz, and then over the last 5 seconds, from 3 to .1.
*/
void CAudioGenerateDoc::OnPracticeA5()
{
	// Call to open the generator output
	if (!GenerateBegin())
		return;

	double startingFrequency = 554.36;							// Hz
	double amplitude = 3000;
	double vibrato = 10;										// Hz
	double radiansSine = 0;										// Sine wave
	double radiansVibrato = 0;									
	double vibratoRate = .1;									// Hz (to 3)
	double vibratoEnd = 3;

	short audio[2];												// Audio channels

	m_amplitude = amplitude;

	// For each frame - while keeping time
	for (double time = 0.; time < m_duration; time += 1. / m_sampleRate)
	{
		// Setting the frequency
		double freq = startingFrequency;

		// Creating sine wave
		short sample = short(m_amplitude * sin(radiansSine));
		audio[0] = audio[1] = sample;

		// Changing vibrato
		if (time < 5)
			vibratoRate = (time / 5) * vibratoEnd;
		else if (time >= 5)
			vibratoRate = (1 - ((time - 5) / 5)) * vibratoEnd;

		double diff = sin(radiansVibrato) * vibratoRate;

		// Increment phases
		radiansSine += (2 * M_PI * (freq + diff)) / m_sampleRate;
		radiansVibrato += (2 * M_PI * vibrato) / m_sampleRate;

		GenerateWriteFrame(audio);

		// The progress control
		if (!GenerateProgress(time / m_duration))
			break;
	}

	// Call to close the generator output
	GenerateEnd();
}


/**
* Creates a 554.36Hz sine wave, at amplitude 3000, and a 10Hz tremolo (amplitude). Over first five seconds
*		the tremolo RATE goes from .1 to 5Hz, and then over the last 5 seconds, from 5 to .1.
*/
void CAudioGenerateDoc::OnPracticeA6()
{
	// Call to open the generator output
	if (!GenerateBegin())
		return;

	double startingFrequency = 587.33;							// Hz
	double amplitude = 3000;
	double tremolo = .1;										// Percentage
	double radiansSine = 0;										// Sine wave
	double radiansTremolo = 0;
	double tremoloRate = 2;										// Hz (to 5)
	double tremoloEnd = 5;

	short audio[2];												// Audio channels

	m_amplitude = amplitude;

	// For the duration
	for (double time = 0.; time < m_duration; time += 1. / m_sampleRate)
	{
		double freq = startingFrequency;

		// Calculate difference in volume (adding 1, because we want it to be +- 10% of our amplitude - need it to be .9 or 1.1)
		double diff = 1 + sin(radiansTremolo);

		// Create our main sine wave - amplitude multiplied by the difference in tremolo (1.1 ... 1.01 .. etc)
		short sample = short((m_amplitude * diff) * sin(radiansSine));
		audio[0] = audio[1] = sample;

		// Changing tremolo rate based on time
		if (time < 5)
			tremoloRate = (time / 5) * tremoloEnd;
		else if (time >= 5)
			tremoloRate = (1 - ((time - 5) / 5)) * tremoloEnd;

		// Increment phases
		radiansSine += (2 * M_PI * freq) / m_sampleRate;
		radiansTremolo += (2 * M_PI * tremoloRate) / m_sampleRate;	// Our tremolo wave
		

		GenerateWriteFrame(audio);

		// The progress control
		if (!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();
}

/**
*	Generates a square wave using time, and saves to a wavetable via pushback.
*	Reads from the wavetable by iteration and plays the sound over 5 seconds.
*/
void CAudioGenerateDoc::OnPracticeA7()
{
	// Call to open the generator output
	if (!GenerateBegin())
		return;

	double startingFrequency = 882;
	double nyquistFrequency = m_sampleRate / 2;
	double duration = 1;										// Second

	std::vector<short> wavetable;

	short audio[2];												// Audio channels

	// Reading the square wave into wt
	for (double time = 0; time <= duration; time += 1. / SampleRate()) {
		short sample = m_amplitude * sin((time * 2 * M_PI * startingFrequency));

		for (int i = 3; (i * startingFrequency) < nyquistFrequency; i += 2) {
			sample += short((m_amplitude / i) * sin((time * 2 * M_PI * i * startingFrequency)));
		}

		wavetable.push_back(sample);
	}

	// Setting duration to 5 sec
	m_duration = 5;
	int p = 0;
	// Playing the square wave from wavetable
	for (double time = 0; time < m_duration; time += 1. / SampleRate()) {
		audio[0] = audio[1] = wavetable[p];

		p++;
		if (p >= wavetable.size())
			p = 0;

		GenerateWriteFrame(audio);

		// The progress control
		if (!GenerateProgress(time / m_duration))
			break;
	}

	// Call to close the generator output
	GenerateEnd();
}

/**
*	Generates a sawtooth wave using time, and saves to a wavetable via pushback.
*	Reads from the wavetable by iteration and plays the sound over 5 seconds.
*/
void CAudioGenerateDoc::OnPracticeA8()
{
	// Call to open the generator output
	if (!GenerateBegin())
		return;

	double startingFrequency = 882;
	double nyquistFrequency = m_sampleRate / 2;
	double duration = 1;										// Second

	std::vector<short> wavetable;

	short audio[2];												// Audio channels

	// Reading the square wave into wt
	for (double time = 0; time <= duration; time += 1. / SampleRate()) {
		short sample = m_amplitude * sin((time * 2 * M_PI * startingFrequency));

		for (int i = 2; (i * startingFrequency) < nyquistFrequency; i++) {
			sample += short((m_amplitude / i) * sin((time * 2 * M_PI * i * startingFrequency)));
		}

		wavetable.push_back(sample);
	}

	// Setting duration to 5 sec
	m_duration = 5;
	int p = 0;
	// Playing the square wave from wavetable
	for (double time = 0; time < m_duration; time += 1. / SampleRate()) {
		audio[0] = audio[1] = wavetable[p];

		p++;
		if (p >= wavetable.size())
			p = 0;

		GenerateWriteFrame(audio);

		// The progress control
		if (!GenerateProgress(time / m_duration))
			break;
	}

	// Call to close the generator output
	GenerateEnd();
}

/* ======================================================================== *\
!
!   Author(s): Daniel Kerszberg         01/2019 <mailto:dkerszberg@ifae.es>
!   Author(s): Chiara Giuri             01/2019 <mailto:chiara.giuri@desy.de>
!
\* ======================================================================== */

//////////////////////////////////////////////////////////////////////////////
//						
// GloryDuckTables2019Lkl
//
// This class takes as input a list of values of -2logL vs <sv> for 
// different masses (in the framework of the Glory Duck project).
// The likelihood values are already using the correct units, 
// therefore SetUnitsOfG is deactivated for this class.
// 
// Usage example:
// --------------
//
// GloryDuckTables2019Lkl *Lkl = new GloryDuckTables2019Lkl(inputString);
// Lkl.PlotInputData();
//
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "TFile.h"
#include "TMath.h"
#include "TPRegexp.h"

#include "GloryDuckTables2019Lkl.h"
#include "ParabolaLkl.h"

ClassImp(GloryDuckTables2019Lkl);

using namespace std;

// static constants
static const TString  gName            = "GloryDuckTables2019Lkl";
static const TString  gTitle           = "<sv> vs mass";
static const Int_t    gNPars            = 1;             // Number of free+nuisance parameters
static const Char_t*  gParName[gNPars]  = {"g"};         // Name of parameters
static const Double_t gRefsv            = 1.e-26;        // [cm^3 s^-1] (reference sv)

// -2logL function for minuit
void gloryDuckTables2019Lkl(Int_t &fpar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag);

// static minuit object for passing class info into the -2logL function
static TMinuit* minuit = NULL;

////////////////////////////////////////////////////////////////
// 
// Default constructor
// inputfilename is the name of a file with the input data
// (i.e. logL vs <sv> for several DM masses)
//
GloryDuckTables2019Lkl::GloryDuckTables2019Lkl(TString inputString) :
  Lkl(gNPars,inputString,gName,gTitle), fMass(NULL), fNMasses(0),
 fsvMin(NULL), fsvMax(NULL), fLogJ(0), fSampleArray(NULL)
{
  if(InterpretInputString(inputString))
    cout << "GloryDuckTables2019Lkl::GloryDuckTables2019Lkl Warning: there were problems interpreting the input string" << endl;
}
//////////////////////////////////////////////////////////////////
//
// Interpret the input configuration string
// The string has been passed to Lkl::InterpretInputString in constructor
// here it is searched for the following options:
//
// inputfile=<val>:     name of the input file, which contains the lkl vs <sv> tables
//
Int_t GloryDuckTables2019Lkl::InterpretInputString(TString inputString)
{
  // file name and path default values
  TString inputfileName = " (No file has been specified) ";
  TString path          = "";

  // Prepeare to interpret inputString
  TPMERegexp re("\\s+");
  TPMERegexp fldre("=");
  
  // split the inputString into the different fields, and check the option and values specified in each of them
  UInt_t nfields = re.Split(inputString);
  for(UInt_t ifield=0;ifield<nfields;ifield++)
    {
      fldre.Split(re[ifield]);
      TString optname = fldre[0];
      if(optname.CompareTo("inputfile",TString::kIgnoreCase)==0)
	inputfileName=fldre[1];
      else if(optname.CompareTo("path",TString::kIgnoreCase)==0)
	path=fldre[1];    
    }

  fSampleArray = new TObjArray();

  // open and read input file
  return ReadGloryDuckInputData(path+"/"+inputfileName);
}

////////////////////////////////////////////////////////////////
// 
// Destructor
//
GloryDuckTables2019Lkl::~GloryDuckTables2019Lkl()
{
  if(fSampleArray) delete fSampleArray;
  if(fsvMin)       delete [] fsvMin;
  if(fsvMax)       delete [] fsvMax;
  if(fMass)        delete [] fMass;
}

////////////////////////////////////////////////////////////////
//
// Link fMinuit to the static global variable minuit (otherwise 
// it won't work because of minuit subtleties)
//
void GloryDuckTables2019Lkl::SetMinuitLink()
{
  minuit = fMinuit; // copy the pointer so that it can be used by FCN 
}

////////////////////////////////////////////////////////////////
//
// Pass the -2logL function to fMinuit,
// initialize the parameters (free+nuisance),
//
void GloryDuckTables2019Lkl::SetFunctionAndPars(Double_t ginit)
{
  fMinuit->SetFCN(gloryDuckTables2019Lkl);
  fMinuit->SetName(Form("%s_Minuit",GetName()));

  // Initialize minuit for added objects
  TObjArrayIter* iter    = (TObjArrayIter*) GetSampleArray()->MakeIterator();
  Lkl* sample;
  while((sample=(Lkl*)iter->Next()))
    {
      sample->InitMinuit();
      sample->SetMinuitLink();
    }
  delete iter;

  Double_t pStart[gNPars] = {(ginit==0? gRefsv :ginit)};
  Double_t pDelta[gNPars] = {1};

  SetParameters(gParName,pStart,pDelta);
}  

////////////////////////////////////////////////////////////////
//
// Check that everything is correct
//
// Return 0 in case of success, 1 otherwise
//
Int_t GloryDuckTables2019Lkl::MakeChecks()
{
  if(IsChecked()) return 0;

  // Check all parabolas
  TObjArrayIter* iter = (TObjArrayIter*) GetSampleArray()->MakeIterator();
  Lkl* sample;
  iter->Reset();
  while((sample=(Lkl*) iter->Next()))
    if(!sample->IsChecked())
      sample->MakeChecks();
  delete iter;

  SetChecked();
  return 0;
}

////////////////////////////////////////////////////////////////
//
// Disable SetUnitsOfG, since units are used internally by the 
// class and should not be changed.
//
void GloryDuckTables2019Lkl::SetUnitsOfG(Double_t units)
{
  cout<< "GloryDuckTables2019Lkl::SetUnitsOfG (" << GetName() << ") Warning: cannot set units for GloryDuckTables2019Lkl." << endl;
}

////////////////////////////////////////////////////////////////
//
// Read the Glory Duck input file and add the 
// ParabolaLkl describing the -2logL function vs <sv> 
//
// Return 0 in case of success, 1 otherwise
//
Int_t GloryDuckTables2019Lkl::ReadGloryDuckInputData(TString filename)
{
  // try to open file
  ifstream ff(filename.Data());
  if(!ff.is_open())
    {
      cout << "GloryDuckTables2019Lkl::ReadGloryDuckInputData (" << GetName() << ") Error: could not open file " << filename << endl;
      return 1;
    }
  else
    cout << "GloryDuckTables2019Lkl::ReadGloryDuckInputData (" << GetName() << ") Message: Reading masses and <sigmav> values from file " << filename << endl;

  vector<Double_t> vsigma ;    // vector with <sv> values
  vector<Double_t> vlkl ;      // vecot with -2logL values

  // get first line of the file (with <sv> values)
  string sigmavLine;
  getline(ff,sigmavLine);

  // read first line of the file
  std::istringstream row(sigmavLine);
  Double_t field;
  while (row>> field)
    {
      vsigma.push_back(field);
    }

  Double_t readingMass = 0.;
  Double_t readingLkl = 0.;
  Int_t counter = 0;

  // get mass (first column)
  while(ff >> readingMass)
    {
      // get rest of the line (<sv> values)
      string LklLine;
      getline(ff,LklLine);

      // read rest of the line
      std::istringstream line(LklLine);
      while (line >> readingLkl)
        {
          vlkl.push_back(readingLkl);
        }

      // add new parabola
      CreateAndAddNewParabola(readingMass,vsigma[0],vsigma[vsigma.size()-1],vsigma.size(),vsigma.data(),vlkl.data());

      // check if line was complete or not
      if(vlkl.size() != vsigma.size())
        {
          cout << "You have a different line size length for line " << counter+1 << " ! Number of sigmav values = " << vsigma.size() << " while number of likelihood values = " << vlkl.size() << endl; 
        }
      counter++;
      vlkl.clear();
   }

  ff.close(); 

  SetChecked(kFALSE);

  cout << "GloryDuckTables2019Lkl::ReadGloryDuckInputData (" << GetName() << ") Message: Read " <<  GetSampleArray()->GetEntries() << " parabolas for a total of " << fNMasses << " DM masses." << endl;
  return 0;
}

////////////////////////////////////////////////////////////////
//
// Add a new ParabolaLkl object representing the -2logL values vs
// <sv> for one DM mass
//
// Return 0 in case of success, 1 otherwise
//
Int_t GloryDuckTables2019Lkl::CreateAndAddNewParabola(Double_t mass,Double_t svmin,Double_t svmax,Int_t npoints,Double_t* sv,Double_t* logL) 
{
  fNMasses++;

  // Add new bin bounds to the list
  Double_t* newMassList = new Double_t[fNMasses];
  Double_t* newSvMinList = new Double_t[fNMasses];
  Double_t* newSvMaxList = new Double_t[fNMasses];

  // copy old lists into new ones
  if(fNMasses>1)
    {
      for(Int_t ibin=0;ibin<fNMasses-1;ibin++)
	{
	  newMassList[ibin] = fMass[ibin];
	  newSvMinList[ibin] = fsvMin[ibin];
	  newSvMaxList[ibin] = fsvMax[ibin];
	}
      delete [] fMass;
      delete [] fsvMin;
      delete [] fsvMax;
    }

  newMassList[fNMasses-1] = mass;
  newSvMinList[fNMasses-1] = svmin;
  newSvMaxList[fNMasses-1] = svmax;
  fMass = newMassList;
  fsvMin = newSvMinList;
  fsvMax = newSvMaxList;

  // Create and add the new ParabolaLkl object
  ParabolaLkl* newparabola = new ParabolaLkl(npoints,sv,logL,Form("ParabolaLkl_%d",GetSampleArray()->GetEntries()));
  AddSample(newparabola);
  
  SetChecked(kFALSE);
  return 0;
}

//////////////////////////////////////////////////////////////////
//
// Function called by Lkl::PrintOverview
// Print info about the experimental data
//
void GloryDuckTables2019Lkl::PrintData(Int_t level)
{
  Lkl::PrintData(level);

  Margin(level); cout << "                of masses = " << fNMasses <<endl;
  Margin(level); cout << "                 <sv>_min = " << fsvMin[0]        << " [cm^3 s^-1]" << endl;
  Margin(level); cout << "                 <sv>_max = " << fsvMax[fNMasses-1] << " [cm^3 s^-1]" << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Plot a canvas with all the -2logL, ie an array
// of graphs (1 per mass) with -2logL vs <sv>
// 
// Return a pointer to the canvas, in case you want to modify it
//
TCanvas* GloryDuckTables2019Lkl::PlotInputData() 
{
  const Int_t nlines = 5;  // number of lines in parabolas canvas
  Int_t ncols = TMath::Ceil(fNMasses/Float_t(nlines)); // number of columns in parabolas canvas

  // create and divide canvas
  TCanvas* canvas = new TCanvas("inputDataCanvas","-2logL value per energy bin and energy flux",ncols*250,nlines*250);
  canvas->Divide(ncols,nlines);

  for(Int_t ibin=0;ibin<fNMasses;ibin++)
    {
      ParabolaLkl* prbla   = (ParabolaLkl*)GetSample(ibin);
      TGraph*      grprbla = prbla->GetParabola(kFALSE);

      canvas->cd(ibin+1);
      TH1I *dummypr = new TH1I(Form("dummypr_%d",ibin),Form("-2logLkl vs <sv> for m_{DM}=%.1f \\ and <sv>=[%.1f,%.1f] [cm^3 s^-1]",fMass[ibin],fsvMin[ibin],fsvMax[ibin]),1,grprbla->GetX()[1]*0.1,grprbla->GetX()[grprbla->GetN()-1]);
      dummypr->SetDirectory(0);
      dummypr->SetStats(0);
      dummypr->SetXTitle("<sv> [cm^3 s^-1]");
      dummypr->SetYTitle("#Delta(-2logL)");
      dummypr->SetMinimum(0);
      dummypr->SetMaximum(10);
      dummypr->DrawCopy();
      gPad->SetLogx();
      delete dummypr;

      // plot -2logLkl vs <sv> 
      grprbla->Draw("l");
      gPad->SetGrid();
      gPad->Modified();
      gPad->Update();
    }
  return canvas;
}

////////////////////////////////////////////////////////////////////////
//
// Set mass index to the current active one
//
void GloryDuckTables2019Lkl::SetActiveMass(Double_t mass)
{
  for (Int_t counter = 0; counter < fNMasses; counter++)
    {
      if(TMath::Abs(fMass[counter]-mass) < 1e-3)
        {
          fActiveMass = counter;
          return;
        }
    }

  cout << "GloryDuckTables2019Lkl::SetActiveMass() Message: Mass " << mass << " doesn't exist." << endl;
  return;
}

////////////////////////////////////////////////////////////////////////
//
// Likelihood function (-2logL) for one mass
// To be minimized by TMinuit
// Free parameters:
// par[0] = g
//
void gloryDuckTables2019Lkl(Int_t &fpar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag)
{
  // to avoid warnings only
  gin[0]*=1;
  fpar*=1;
  iflag*=1;
  f = 0;

  // get internal object
  GloryDuckTables2019Lkl* mylkl = dynamic_cast<GloryDuckTables2019Lkl*>(minuit->GetObjectFit());

  // g is the global scale for dPhidESignal
  Double_t g = par[0];

  // get likelihood for current active mass
  Lkl* sample = mylkl->GetSample(mylkl->GetActiveMass());
  //ParabolaLkl* sample = (ParabolaLkl*)mylkl->GetSample(mylkl->GetActiveMass());

  // minimize
  f=sample->MinimizeLkl(g,kTRUE,kFALSE);
}

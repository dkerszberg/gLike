//######################################################################
//## convert file containing (old) MIACTEventListIRF object
//## into new one with the equivalent IactEventListIrf object
//##
//## Run macro in the ROOT command line as:
//##
//## convertGLikeInputFiles(<oldfilename>,<newfilename>)
//##
//## to convert from file <oldfilename> into <newfilename>
//######################################################################

#include <iostream>
#include<vector>

#include "TROOT.h"
#include "TString.h"
#include "TFile.h"
#include "TRandom.h"
#include "TRandom2.h"

#include "MIACTEventListIRF.h"
#include "IactEventListIrf.h"

using namespace std;

void processSample(TString inFileName,TString outFileName);
void checkFile(TString outputFileName);

void convertGLikeInputFiles(TString inputfile, TString outputfile)
{
  gROOT->SetDirLevel();
  
  if(inputfile.CompareTo("")==0 || outputfile.CompareTo("")==0)
    cout << "Input/output file names cannot be empty" << endl;
  else if(inputfile.CompareTo(outputfile)==0)
    cout << "Input and output file names cannot be the same" << endl;
  else
    {cout <<"first"<<flush<<endl; 
      processSample(inputfile,outputfile);
cout <<"first2"<<flush<<endl; 
      checkFile(outputfile);
cout <<"first3"<<flush<<endl; 
    }
}

void checkFile(TString iFileName)
{
  // open files
  cout << endl;
  cout << "##################################################" << endl;
  cout << "Examining contents of file " << iFileName << endl;
  TFile* ifile = new TFile(iFileName,"READ");
  IactEventListIrf* dataSet = (IactEventListIrf*) ifile->Get("IactEventListIrf");

  // link with NTuple branches
  IactEvent_t eventOn,eventOff;
  dataSet->SetOnBranchAddress("E",&eventOn.E);
  dataSet->SetOnBranchAddress("pointRA",&eventOn.pointRA);
  dataSet->SetOnBranchAddress("pointDEC",&eventOn.pointDEC);
  dataSet->SetOnBranchAddress("dRA",&eventOn.dRA);
  dataSet->SetOnBranchAddress("dDEC",&eventOn.dDEC);
  dataSet->SetOnBranchAddress("t",&eventOn.t);
  dataSet->SetOnBranchAddress("had",&eventOn.had);
  
  dataSet->SetOffBranchAddress("E",&eventOff.E);
  dataSet->SetOffBranchAddress("pointRA",&eventOff.pointRA);
  dataSet->SetOffBranchAddress("pointDEC",&eventOff.pointDEC);
  dataSet->SetOffBranchAddress("dRA",&eventOff.dRA);
  dataSet->SetOffBranchAddress("dDEC",&eventOff.dDEC);
  dataSet->SetOffBranchAddress("t",&eventOff.t);
  dataSet->SetOffBranchAddress("had",&eventOff.had);

  // dump values
  cout << "Epmin    = " << dataSet->GetEpmin() << " GeV" << endl;
  cout << "Epmax    = " << dataSet->GetEpmax() << " GeV" << endl;
  cout << "Tau      = " << dataSet->GetTau() << " +/- " << dataSet->GetDTau() << endl;
  cout << "Obs time = " << dataSet->GetObsTime()/60./60. << " h" << endl;
  cout << endl;
  cout << "First 10 ON events (out of " << dataSet->GetOnSample()->GetEntries() << "): " << endl;
  cout << Form(" E [GeV]\t pointRA [deg]\t pointDEC [deg]\t dRA [deg]\t dDEC [deg]\t t [MJD]\thad") << endl;
  cout << "*****************************************************************************************************" << endl; 
  for(Int_t i=0;i<10;i++)
    {
      dataSet->GetOnEntry(i);
      cout << Form("%.2f\t\t%.1f\t\t%.1f\t\t%.1f\t\t%.1f\t\t%.1f\t\t%.1f",eventOn.E,eventOn.pointRA,eventOn.pointDEC,eventOn.dRA,eventOn.dDEC,eventOn.t,eventOn.had) << endl;
    }
  cout << endl;
  cout << "First 10 OFF events (out of " << dataSet->GetOffSample()->GetEntries() << "): " << endl;
  cout << Form(" E [GeV]\t pointRA [deg]\t pointDEC [deg]\t dRA [deg]\t dDEC [deg]\t t [MJD]\thad") << endl;
  cout << "*****************************************************************************************************" << endl; 
  for(Int_t i=0;i<10;i++)
    {
      dataSet->GetOffEntry(i);
      cout << Form("%.2f\t\t%.1f\t\t%.1f\t\t%.1f\t\t%.1f\t\t%.1f\t\t%.1f",eventOff.E,eventOff.pointRA,eventOff.pointDEC,eventOff.dRA,eventOff.dDEC,eventOff.t,eventOff.had) << endl;
    }

  // look for existing IRF histos/graphs
  
  cout << endl;
  cout << "Checking different IRF histograms/graphs:" << endl;
  cout << "*****************************************" << endl; 
  cout << "Aeff      : " << (dataSet->GetHAeff()->GetEntries()?     "YES" : "NO") << endl;
  cout << "AeffOff   : " << (dataSet->GetHAeffOff()->GetEntries()?  "YES" : "NO") << endl;
  cout << "Eres      : " << (dataSet->GetGEreso()->GetN()?          "YES" : "NO") << endl;
  cout << "Ebias     : " << (dataSet->GetGEbias()->GetN()?          "YES" : "NO") << endl;
  cout << "MigMatrix : " << (dataSet->GetMigMatrix()->GetEntries()? "YES" : "NO") << endl;
  cout << "dNdEpBkg  : " << (dataSet->GetHdNdEpBkg()->GetEntries()? "YES" : "NO") << endl;
  cout << "dNdEpFrg  : " << (dataSet->GetHdNdEpFrg()->GetEntries()? "YES" : "NO") << endl;

  // close and clean
  delete dataSet;
  ifile->Close();
  delete ifile;
}


void processSample(TString inputFileName,TString outputFileName)
{
  cout << "Converting file " << inputFileName << " into " << outputFileName << "... " << flush;
  TFile *file0 = TFile::Open(inputFileName);

  //MIACTEventListIRF* oldclass = (MIACTEventListIRF*) file0->Get("MIACTEventListIRF");
  IactEventListIrf* oldclass = (IactEventListIrf*) file0->Get("IactEventListIrf");

   // link with NTuple branches
  IactEvent_t dummy;
  IactEvent_t eventOn;
  IactEvent_t eventOff;
  oldclass->SetOnBranchAddress("E",&eventOn.E);
  oldclass->SetOnBranchAddress("dRA",&eventOn.dRA);
  oldclass->SetOnBranchAddress("dDEC",&eventOn.dDEC);
  oldclass->SetOnBranchAddress("t",&eventOn.t);
  oldclass->SetOnBranchAddress("had",&eventOn.had);
  
  oldclass->SetOffBranchAddress("E",&eventOff.E);
  oldclass->SetOffBranchAddress("dRA",&eventOff.dRA);
  oldclass->SetOffBranchAddress("dDEC",&eventOff.dDEC);
  oldclass->SetOffBranchAddress("t",&eventOff.t);
  oldclass->SetOffBranchAddress("had",&eventOff.had);

  Double_t myE=0.;
  Double_t myT=0.;
  Double_t myTprime=0.;

  TRandom2 *rand =new TRandom2(3);
  //vector<double> vect_on;
  vector< pair <double,double> >vect_on;
  vector<double> vect_on_E;
  vector<double> vect_off;
cout <<"first"<<endl; 
  // read event data from old class and write it in new class
  IactEventListIrf*  newclass = new IactEventListIrf;
  UInt_t non = 2000;//oldclass->GetOnSample()->GetEntries();
  for(UInt_t ion=0;ion<non;ion++)
    {
      myT = rand->Gaus(600,150)/86400. + 50000;
      //vect_on.push_back(myT);
      myE = 10000*TMath::Power(rand->Uniform(1,10),-2);
      myTprime=myT + 10*0.017*myE/86400.;
      //vect_on_E.push_back(myE);
      vect_on.push_back(make_pair(myTprime,myE));
    }
  UInt_t non_2 = 3000;//oldclass->GetOnSample()->GetEntries();
  for(UInt_t ion=0;ion<non_2;ion++)
    {
      myT = rand->Uniform(50,1150)/86400. + 50000;
     // vect_on.push_back(myT);
      myE = 10000*TMath::Power(rand->Uniform(1,10),-2.7);
      //vect_on_E.push_back(myE);
      vect_on.push_back(make_pair(myT,myE));
    }
cout <<"before"<<endl; 
  sort(vect_on.begin(),vect_on.end());
cout <<"after"<<endl;
  for(UInt_t ion=0;ion<non+non_2;ion++)
    {
      oldclass->GetOnEntry(ion);
      myTprime = vect_on[ion].first; //rand->Gaus(600,150)/86400. + 50000;
      myE = vect_on[ion].second; //rand->Gaus(600,150)/86400. + 50000;
      newclass->FillOnEvent(myE,IactEventListIrf::gDefRADECVal,IactEventListIrf::gDefRADECVal,eventOn.dRA,eventOn.dDEC,myTprime,eventOn.had);
    }

  UInt_t noff = 3000; //oldclass->GetOffSample()->GetEntries();
  for(UInt_t ioff=0;ioff<noff;ioff++)
    {
      myT = rand->Uniform(50,1150)/86400. + 50000;
      vect_off.push_back(myT);
    }
  sort(vect_off.begin(),vect_off.end());
  for(UInt_t ioff=0;ioff<noff;ioff++)
    {
      oldclass->GetOffEntry(ioff);
      myT = vect_off[ioff]; //rand->Uniform(50,1150)/86400. + 50000;
      myE = 10000*TMath::Power(rand->Uniform(1,10),-2.7);
      newclass->FillOffEvent(myE,IactEventListIrf::gDefRADECVal,IactEventListIrf::gDefRADECVal,eventOff.dRA,eventOff.dDEC,myT,eventOff.had);
    }

  // save the rest of the class member data values
  //newclass->SetEnergyRange(oldclass->GetEpmin(),oldclass->GetEpmax());
  newclass->SetEnergyRange(oldclass->GetEpmin(),oldclass->GetEpmax());
  newclass->SetTau(oldclass->GetTau(),oldclass->GetDTau(),oldclass->GetTauPValue());
  newclass->SetObsTime(oldclass->GetObsTime());
  newclass->SetHAeff(oldclass->GetHAeff());
  newclass->SetHAeffOff(oldclass->GetHAeffOff());
  newclass->SetGEResoAndBias(oldclass->GetGEreso(),oldclass->GetGEbias());
  newclass->SetMigMatrix(oldclass->GetMigMatrix());
  newclass->SetHdNdEpBkg(oldclass->GetHdNdEpBkg());
  newclass->SetHdNdEpFrg(oldclass->GetHdNdEpFrg());
  
  
  TFile *file1 = TFile::Open(outputFileName,"RECREATE");
  newclass->Write();
  file1->Close();
  file0->Close();
  delete file1;
  delete file0;
  
  cout << "Done!" << endl;
}

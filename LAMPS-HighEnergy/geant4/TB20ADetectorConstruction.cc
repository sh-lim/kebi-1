#include "KBParameterContainer.hh"
#include "TB20ADetectorConstruction.hh"

#include "KBG4RunManager.hh"
#include "KBGeoBoxStack.hh"
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserLimits.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4UniformMagField.hh"
#include "G4GlobalMagFieldMessenger.hh"
#include "G4SubtractionSolid.hh"

TB20ADetectorConstruction::TB20ADetectorConstruction()
: G4VUserDetectorConstruction()
{
}

TB20ADetectorConstruction::~TB20ADetectorConstruction()
{
}

G4VPhysicalVolume *TB20ADetectorConstruction::Construct()
{
  auto runManager = (KBG4RunManager *) G4RunManager::GetRunManager();

  auto par = runManager -> GetParameterContainer();

	G4double tpcInnerRadius = par -> GetParDouble("TPCrMin");
	G4double tpcOuterRadius = par -> GetParDouble("TPCrMax");
	G4double tpcLength = par -> GetParDouble("TPCLength");
	G4double tpcZOffset = par -> GetParDouble("TPCzOffset");

  G4NistManager *nist = G4NistManager::Instance();
  G4double STPTemperature = 273.15;
  G4double labTemperature = STPTemperature + 20.*kelvin;
  
	G4Element *elementH = new G4Element("elementH", "H", 1., 1.00794*g/mole);
	G4Element *elementC = new G4Element("elementC", "C", 6., 12.011*g/mole);
	//G4Element *elementCu = new G4Element("elementCu","Cu", 29., 63.546*g/mole);
	//G4Element *elementAl = new G4Element("elementAl","Al", 13., 26.982*g/mole);

  G4double densityArGas = 1.782e-3*g/cm3*STPTemperature/labTemperature;
  G4Material *matArGas = new G4Material("ArgonGas", 18, 39.948*g/mole, densityArGas, kStateGas, labTemperature);

	//G4Material *matCH2 = new G4Material("CH2", 0.91*g/cm3, 2);
	//matCH2->AddElement(elementH,2);
	//matCH2->AddElement(elementC,1);

	G4Material *matAl = nist->FindOrBuildMaterial("G4_Al");
	G4Material *matCu = nist->FindOrBuildMaterial("G4_Cu");
	G4Material *matFe = nist->FindOrBuildMaterial("G4_Fe");
	G4Material *matSC = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
	G4Material *matCH2 = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
	G4Material *matAir = nist->FindOrBuildMaterial("G4_AIR");
	G4Material *matVac = nist->FindOrBuildMaterial("G4_Galactic");
 
  G4double densityMethane = 0.717e-3*g/cm3*STPTemperature/labTemperature;
  G4Material *matMethaneGas = new G4Material("matMethaneGas ", densityMethane, 2, kStateGas, labTemperature);
  matMethaneGas -> AddElement(elementH, 4);
  matMethaneGas -> AddElement(elementC, 1);

  TString gasPar = "p10";
  if (par -> CheckPar("TPCgasPar")) {
    gasPar = par -> GetParString("TPCgasPar");
    gasPar.ToLower();
         if (gasPar.Index("p10")>=0) gasPar = "p10";
    else if (gasPar.Index("p20")>=0) gasPar = "p20";
    else gasPar = "p10";
  }

  G4Material *matGas = nullptr;
  if (gasPar == "p10") {
    G4double densityGas = .9*densityArGas + .1*densityMethane;
    matGas = new G4Material("matP10", densityGas, 2, kStateGas, labTemperature);
    matGas -> AddMaterial(matArGas, 0.9*densityArGas/densityGas);
    matGas -> AddMaterial(matMethaneGas, 0.1*densityMethane/densityGas);
  }
  else if (gasPar == "p20") {
    G4double densityGas = .8*densityArGas + .2*densityMethane;
    matGas = new G4Material("matP20", densityGas, 2, kStateGas, labTemperature);
    matGas -> AddMaterial(matArGas, 0.8*densityArGas/densityGas);
    matGas -> AddMaterial(matMethaneGas, 0.2*densityMethane/densityGas);
  }

  G4double worlddX = par -> GetParDouble("worlddX");
  G4double worlddY = par -> GetParDouble("worlddY");
  G4double worlddZ = par -> GetParDouble("worlddZ");

	//world
  G4Box *solidWorld = new G4Box("World", worlddX, worlddY, worlddZ);
  G4LogicalVolume *logicWorld;
	if ( par -> GetParInt("worldOpt")==0 ){
		logicWorld = new G4LogicalVolume(solidWorld, matVac, "World");
	}else if ( par -> GetParInt("worldOpt")==1 ){
		logicWorld = new G4LogicalVolume(solidWorld, matAir, "World");
	}else{
		logicWorld = new G4LogicalVolume(solidWorld, matVac, "World");
	}
  G4PVPlacement *physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, -1, true);


	//Collimator
	if ( par -> GetParBool("CollimatorIn") )
	{
		G4double Collimatorx = par -> GetParDouble("Collimatorx");
		G4double Collimatory = par -> GetParDouble("Collimatory");
		G4double Collimatorz = par -> GetParDouble("Collimatorz");
		G4double CollimatorzOffset = par -> GetParDouble("CollimatorzOffset");

		G4Box *solidCollimator = new G4Box("Collimator", Collimatorx/2.0, Collimatory/2.0, Collimatorz/2.0);
		G4SubtractionSolid *solidSubC;
		if ( par -> GetParInt("HoleOpt")==0 ){
			G4double Holex = par -> GetParDouble("Holex");
			G4double Holey = par -> GetParDouble("Holey");
			G4Box *solidHole = new G4Box("Hole", Holex/2.0, Holey/2.0, Collimatorz/2.0);
			solidSubC = new G4SubtractionSolid("SubC", solidCollimator, solidHole, 0, G4ThreeVector(0,0,0));
		}else if ( par -> GetParInt("HoleOpt")==1 ){
			G4double Holedia = par -> GetParDouble("Holedia");
			G4Tubs *solidHole = new G4Tubs("Hole", 0, Holedia/2, Collimatorz/2.0, 0, 2*M_PI);
			solidSubC = new G4SubtractionSolid("SubC", solidCollimator, solidHole, 0, G4ThreeVector(0,0,0));
		}else{
			G4double Holex = par -> GetParDouble("Holex");
			G4double Holey = par -> GetParDouble("Holey");
			G4Box *solidHole = new G4Box("Hole", Holex/2.0, Holey/2.0, Collimatorz/2.0);
			solidSubC = new G4SubtractionSolid("SubC", solidCollimator, solidHole, 0, G4ThreeVector(0,0,0));
		}

		G4LogicalVolume *logicSubC;
		if ( par -> GetParInt("CollimatorOpt")==0 ){
			logicSubC = new G4LogicalVolume(solidSubC, matAl, "SubC");
		}else if ( par -> GetParInt("CollimatorOpt")==1 ){
			logicSubC = new G4LogicalVolume(solidSubC, matCu, "SubC");
		}else if ( par -> GetParInt("CollimatorOpt")==2 ){
			logicSubC = new G4LogicalVolume(solidSubC, matFe, "SubC");
		}else{
			logicSubC = new G4LogicalVolume(solidSubC, matAl, "SubC");
		}

		{
			G4VisAttributes * attSubC = new G4VisAttributes(G4Colour(G4Colour::Yellow()));
			logicSubC -> SetVisAttributes(attSubC);
		}
		new G4PVPlacement(0, G4ThreeVector(0,0,CollimatorzOffset), logicSubC, "SubC", logicWorld, false, 0, true);
	}

	if ( par -> GetParBool("Collimator2In") )
	{
		G4double Collimatorx = par -> GetParDouble("Collimatorx");
		G4double Collimatory = par -> GetParDouble("Collimatory");
		G4double Collimatorz = par -> GetParDouble("Collimatorz");
		G4double CollimatorzOffset = par -> GetParDouble("CollimatorzOffset") + par -> GetParDouble("Collimator2zGap") + Collimatorz;

		G4Box *solidCollimator2 = new G4Box("Collimator2", Collimatorx/2.0, Collimatory/2.0, Collimatorz/2.0);
		G4SubtractionSolid *solidSubC2;
		if ( par -> GetParInt("HoleOpt")==0 ){
			G4double Holex = par -> GetParDouble("Holex");
			G4double Holey = par -> GetParDouble("Holey");
			G4Box *solidHole = new G4Box("Hole", Holex/2.0, Holey/2.0, Collimatorz/2.0);
			solidSubC2 = new G4SubtractionSolid("SubC", solidCollimator2, solidHole, 0, G4ThreeVector(0,0,0));
		}else if ( par -> GetParInt("HoleOpt")==1 ){
			G4double Holedia = par -> GetParDouble("Holedia");
			G4Tubs *solidHole = new G4Tubs("Hole", 0, Holedia/2, Collimatorz/2.0, 0, 2*M_PI);
			solidSubC2 = new G4SubtractionSolid("SubC", solidCollimator2, solidHole, 0, G4ThreeVector(0,0,0));
		}else{
			G4double Holex = par -> GetParDouble("Holex");
			G4double Holey = par -> GetParDouble("Holey");
			G4Box *solidHole = new G4Box("Hole", Holex/2.0, Holey/2.0, Collimatorz/2.0);
			solidSubC2 = new G4SubtractionSolid("SubC", solidCollimator2, solidHole, 0, G4ThreeVector(0,0,0));
		}

		G4LogicalVolume *logicSubC2;
		if ( par -> GetParInt("CollimatorOpt")==0 ){
			logicSubC2 = new G4LogicalVolume(solidSubC2, matAl, "SubC");
		}else if ( par -> GetParInt("CollimatorOpt")==1 ){
			logicSubC2 = new G4LogicalVolume(solidSubC2, matCu, "SubC");
		}else if ( par -> GetParInt("CollimatorOpt")==2 ){
			logicSubC2 = new G4LogicalVolume(solidSubC2, matFe, "SubC");
		}else{
			logicSubC2 = new G4LogicalVolume(solidSubC2, matAl, "SubC");
		}

		{
			G4VisAttributes * attSubC2 = new G4VisAttributes(G4Colour(G4Colour::Yellow()));
			logicSubC2 -> SetVisAttributes(attSubC2);
		}
		new G4PVPlacement(0, G4ThreeVector(0,0,CollimatorzOffset), logicSubC2, "SubC2", logicWorld, false, 0, true);
	}

	//Beam Monitor
	if ( par -> GetParBool("BeamMonitorIn") )
	{
		G4double Collimatorz = par -> GetParDouble("Collimatorz");
		G4double CollimatorzOffset = par -> GetParDouble("CollimatorzOffset") + par -> GetParDouble("Collimator2zGap") + Collimatorz;

		G4Box *solidBM = new G4Box("BM", 200/2.0, 200/2.0, 2.0/2.0);
		G4LogicalVolume *logicBM = new G4LogicalVolume(solidBM, matAir, "BM");
		{
			G4VisAttributes * attBM = new G4VisAttributes(G4Colour(G4Colour::White()));
			logicBM -> SetVisAttributes(attBM);
		}
		new G4PVPlacement(0, G4ThreeVector(0,0,CollimatorzOffset+Collimatorz), logicBM, "BM", logicWorld, false, 0, true);
	}

	//Start counter
	if ( par -> GetParBool("StartCounterIn") )
	{
		G4Box *solidSC = new G4Box("SC", 200/2.0, 200/2.0, 2/2.0);
		G4LogicalVolume *logicSC = new G4LogicalVolume(solidSC, matSC, "SC");
		{
			G4VisAttributes * attSC = new G4VisAttributes(G4Colour(G4Colour::Blue()));
			logicSC -> SetVisAttributes(attSC);
		}
		new G4PVPlacement(0, G4ThreeVector(0,0,-150), logicSC, "SC", logicWorld, false, 0, true);
	}

	if ( par -> GetParBool("VetoIn") )
	{
		G4Box *solidVeto = new G4Box("Veto", 200/2.0, 200/2.0, 2/2.0);
		G4LogicalVolume *logicVeto = new G4LogicalVolume(solidVeto, matSC, "Veto");
		{
			G4VisAttributes * attVeto = new G4VisAttributes(G4Colour(G4Colour::Blue()));
			logicVeto -> SetVisAttributes(attVeto);
		}
		new G4PVPlacement(0, G4ThreeVector(0,0,-100), logicVeto, "Veto", logicWorld, false, 0, true);
	}


	//Target1
	if ( par -> GetParBool("Target1In") )
	{
		G4double Target1x = par -> GetParDouble("Target1x");
		G4double Target1y = par -> GetParDouble("Target1y");
		G4double Target1z = par -> GetParDouble("Target1z");
		G4double Target1zOffset = par -> GetParDouble("Target1zOffset");

		G4Box *solidTarget1 = new G4Box("Target1", Target1x/2.0, Target1y/2.0, Target1z/2.0);
		G4LogicalVolume *logicTarget1 = new G4LogicalVolume(solidTarget1, matCH2, "Traget1");
		{
			G4VisAttributes * attTarget1 = new G4VisAttributes(G4Colour(G4Colour::Green()));
			logicTarget1 -> SetVisAttributes(attTarget1);
		}
		new G4PVPlacement(0, G4ThreeVector(0,0,Target1zOffset), logicTarget1, "Target1", logicWorld, false, 0, true);
	}

	//Target2
	if ( par -> GetParBool("Target2In") )
	{
		G4double Target2x = par -> GetParDouble("Target2x");
		G4double Target2y = par -> GetParDouble("Target2y");
		G4double Target2z = par -> GetParDouble("Target2z");
		G4double Target2zOffset = par -> GetParDouble("Target2zOffset");

		G4Box *solidTarget2 = new G4Box("Target2", Target2x/2.0, Target2y/2.0, Target2z/2.0);
		G4LogicalVolume *logicTarget2 = new G4LogicalVolume(solidTarget2, matCH2, "Traget1");
		{
			G4VisAttributes * attTarget2 = new G4VisAttributes(G4Colour(G4Colour::Green()));
			logicTarget2 -> SetVisAttributes(attTarget2);
		}
		new G4PVPlacement(0, G4ThreeVector(0,0,Target2zOffset), logicTarget2, "Target2", logicWorld, false, 0, true);
	}

	//Target3
	if ( par -> GetParBool("Target3In") )
	{
		G4double Target3x = par -> GetParDouble("Target3x");
		G4double Target3y = par -> GetParDouble("Target3y");
		G4double Target3z = par -> GetParDouble("Target3z");
		G4double Target3zOffset = par -> GetParDouble("Target3zOffset");

		G4Box *solidTarget3 = new G4Box("Target3", Target3x/2.0, Target3y/2.0, Target3z/2.0);
		G4LogicalVolume *logicTarget3 = new G4LogicalVolume(solidTarget3, matCH2, "Traget1");
		{
			G4VisAttributes * attTarget3 = new G4VisAttributes(G4Colour(G4Colour::Green()));
			logicTarget3 -> SetVisAttributes(attTarget3);
		}
		new G4PVPlacement(0, G4ThreeVector(0,0,Target3zOffset), logicTarget3, "Target3", logicWorld, false, 0, true);
	}


	//TPC
	if ( par -> GetParBool("TPCIn") )
	{
		G4Tubs *solidTPC = new G4Tubs("TPC", tpcInnerRadius, tpcOuterRadius, .5*tpcLength, 0., 360*deg);
		G4LogicalVolume *logicTPC = new G4LogicalVolume(solidTPC, matGas, "TPC");
		{
			G4VisAttributes * attTPC = new G4VisAttributes(G4Colour(G4Colour::Gray()));
			attTPC -> SetForceWireframe(true);
			logicTPC -> SetVisAttributes(attTPC);
		}
		logicTPC -> SetUserLimits(new G4UserLimits(1.*mm));
		auto pvp = new G4PVPlacement(0, G4ThreeVector(0,0,tpcZOffset), logicTPC, "TPC", logicWorld, false, 0, true);
		runManager -> SetSensitiveDetector(pvp);
	}



	/*
  bool checkWall = par -> CheckPar("numNeutronWall");
  if (checkWall)
  {
    G4Material* scint_mat = nist -> FindOrBuildMaterial("G4_XYLENE");

    G4int numWall = par -> GetParInt("numNeutronWall");
    for (auto iwall = 0; iwall < numWall; ++iwall) {
      auto naStackAxis = par -> GetParAxis(Form("naStackAxis%d",iwall));
      auto naNumStack = par -> GetParInt(Form("naNumStack%d",iwall));
      auto nadX = par -> GetParDouble(Form("nadX%d",iwall));
      auto nadY = par -> GetParDouble(Form("nadY%d",iwall));
      auto nadZ = par -> GetParDouble(Form("nadZ%d",iwall));
      auto naXOffset = par -> GetParDouble(Form("naXOffset%d",iwall));
      auto naYOffset = par -> GetParDouble(Form("naYOffset%d",iwall));
      auto naZOffset = par -> GetParDouble(Form("naZOffset%d",iwall));

      G4Box* solidScint = new G4Box(Form("Scintillator_%d",iwall), 0.5*nadX, 0.5*nadY, 0.5*nadZ);
      G4LogicalVolume* logicScint = new G4LogicalVolume(solidScint, scint_mat, Form("Scintillator_%d",iwall));

      KBGeoBoxStack boxStack(naXOffset,naYOffset,naZOffset,nadX,nadY,nadZ,naNumStack,naStackAxis,KBVector3::kZ);

      for (auto copy = 0; copy < naNumStack; ++copy) {
        Int_t id = 10000+copy+iwall*100;
        G4String name = Form("Scintillator_%d_%d",iwall,copy);
        auto box = boxStack.GetBox(copy);
        auto pos = box.GetCenter();
        G4ThreeVector gpos(pos.X(),pos.Y(),pos.Z());
        auto cpvp = new G4PVPlacement(0, gpos, logicScint, name, logicWorld, false, id, true);
        runManager -> SetSensitiveDetector(cpvp);
      }
    }
  }
	*/

  //G4double bfieldx = par -> GetParDouble("bfieldx");
  //G4double bfieldy = par -> GetParDouble("bfieldy");
  //G4double bfieldz = par -> GetParDouble("bfieldz");
  //new G4GlobalMagFieldMessenger(G4ThreeVector(bfieldx*tesla, bfieldy*tesla, bfieldz*tesla));

  return physWorld;
}
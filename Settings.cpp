#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>
#include <cmath>
#include <memory>

#include "Settings.hpp"
#include "RubyInterpreter.hpp"
#include "fileUtils.hpp"
#include "Exceptions.hpp"

using namespace std ;

#include "RubyScripts.hpp"

namespace microflow
{
	Settings::
	Settings(const std::string simulationDirectoryPath) :
	simulationDirectoryPath_( simulationDirectoryPath )
	{
		/*
			Embedding ruby interpreter - only once, have problems with second load.
		*/
		_rbi = MRubyInterpreter::getMRubyInterpreter () ;
		
		// load below is needed only to read lattice type and set simulation directory
		// Arbitrary Nx, Ny, will be overriden later
		loadConfiguration(10000, 10000, 10000) ; 
	}

	Settings::
	Settings()
	{
		zExpandDepth_ = 0 ;

		_Nx = 0 ;
		_Ny = 0;

		numberOfStepsBetweenVtkSaves_ = 0 ;
		maxNumberOfVtkFiles_ = 0 ;
		numberOfStepsBetweenCheckpointSaves_ = 0 ;
		maxNumberOfCheckpoints_ = 0 ;

		numberOfStepsBetweenErrorComputation_ = 0 ;

		requiredVelocityRelativeError_ = NAN ;
		initialVolumetricMassDensityLB_ = NAN ;
		initialVelocityLB_[0] = NAN ;
		initialVelocityLB_[1] = NAN ;
		initialVelocityLB_[2] = NAN ;
		kinematicViscosityPhysical_ = NAN ;
		reynoldsNumber_ = NAN ;
		initialVolumetricMassDensityPhysical_ = NAN ;
		characteristicLengthLB_ = NAN ;
		characteristicLengthPhysical_ = NAN ;
		characteristicVelocityPhysical_ = NAN ;

		kinematicViscosityLB_ = NAN ;
		latticeTimeStepPhysical_ = NAN ;
		latticeSpacingPhysical_ = NAN ;
		tau_ = NAN ;
		characteristicVelocityLB_ = NAN ;

		_rbi = nullptr ;
	}

	Settings::
	~Settings()
	{
		_rbi = nullptr ;
	}

	bool Settings::
	isGeometryDefinedByPng() const
	{
		return fileExists (getGeometryPngImagePath()) ;
	}

	bool Settings::
	isGeometryDefinedByVti() const
	{
		return fileExists (getGeometryVtiImagePath()) ;
	}

	void Settings::
	loadConfiguration( size_t geometryWidthInCells, 
										 size_t geometryHeightInCells, 
										 unsigned characteristicLengthInCells )
	{
		stringstream ss ;

		// Avoid Ruby warnings
		ss << "if Object.const_defined?(:Nx) then" 
					" Object.send(:remove_const, :Nx) end ;\n" ;
		ss << "if Object.const_defined?(:Ny) then" 
					" Object.send(:remove_const, :Ny) end ;\n" ;
		ss << "if Object.const_defined?(:Ln) then" 
					" Object.send(:remove_const, :Ln) end ;\n" ;
		ss << "if Object.const_defined?(:Configuration_dir) then" 
					" Object.send(:remove_const, :Configuration_dir) end ;\n" ;
		ss << "if Object.const_defined?(:GeometryDirectory) then" 
					" Object.send(:remove_const, :GeometryDirectory) end ;\n" ;
		ss << "if Object.const_defined?(:SEPARATOR) then" 
					" Object.send(:remove_const, :SEPARATOR) end ;\n" ;
		
		ss << "Nx = " << geometryWidthInCells 
			 << " ; Ny = " << geometryHeightInCells << " ;\n" ;
		ss << "Ln = " << characteristicLengthInCells << " ; \n" ;
		ss << "Configuration_dir = \"" 
			 << getSimulationDirectoryPath() << "/params/\" ; \n" ;
		ss << "GeometryDirectory = \"" 
			 << getGeometryDirectoryPath() << "\" ; \n" ;

		_rbi->runScript (ss.str ()) ;

		_rbi->runScript (read_config_rb ) ;
	
		// Belowe we do not cath exceptions because ruby script initialises all
		// global variables.
		latticeArrangementName_ = 
			_rbi->getMRubyVariable<std::string> ("$lattice") ;
		dataTypeName_ =
			_rbi->getMRubyVariable<std::string> ("$data_type") ;
		fluidModelName_ =
			_rbi->getMRubyVariable<std::string> ("$fluid_model") ;
		collisionModelName_ =
			_rbi->getMRubyVariable<std::string> ("$collision_model") ;
		computationalEngineName_ = 
			_rbi->getMRubyVariable<std::string> ("$computational_engine") ;

		zExpandDepth_ = _rbi->getMRubyVariable<unsigned> ("$z_expand_depth") ;

		shouldSaveVelocityLB_ = 
			_rbi->getMRubyVariable<bool>("$vtk_save_velocity_LB") ;
		shouldSaveVelocityPhysical_ =
			_rbi->getMRubyVariable<bool>("$vtk_save_velocity_physical") ;
		shouldSaveVolumetricMassDensityLB_ =
			_rbi->getMRubyVariable<bool>("$vtk_save_rho_LB") ;
		shouldSavePressurePhysical_ =
			_rbi->getMRubyVariable<bool>("$vtk_save_pressure_physical") ;
		shouldSaveNodes_ =
			_rbi->getMRubyVariable<bool>("$vtk_save_nodes") ;
		shouldSaveMassFlowFractions_ =
			_rbi->getMRubyVariable<bool>("$vtk_save_mass_flow_fractions") ;

		requiredVelocityRelativeError_ = _rbi->getMRubyVariable<double>("$err") ;
		kinematicViscosityPhysical_ = _rbi->getMRubyVariable<double>("$nu_phys") ;
		tau_ = _rbi->getMRubyVariable<double> ("$tau") ;

		initialVelocityLB_[ X ] = _rbi->getMRubyVariable<double> ("$ux0_LB") ;
		initialVelocityLB_[ Y ] = _rbi->getMRubyVariable<double> ("$uy0_LB") ;
		initialVelocityLB_[ Z ] = _rbi->getMRubyVariable<double> ("$uz0_LB") ;
		
		characteristicLengthPhysical_ = 
			_rbi->getMRubyVariable<double> ("$l_ch_phys") ;
		characteristicVelocityPhysical_ = 
			_rbi->getMRubyVariable<double> ("$u_ch_phys") ;
		initialVolumetricMassDensityPhysical_ = 
			_rbi->getMRubyVariable<double>("$rho0_phys") ;
		initialVolumetricMassDensityLB_ = 
			_rbi->getMRubyVariable<double>("$rho0_LB") ;

		_Nx = _rbi->getMRubyVariable<unsigned int> ("$Nx") ;
		_Ny = _rbi->getMRubyVariable<unsigned int> ("$Ny") ;

		numberOfStepsBetweenVtkSaves_ = 
			_rbi->getMRubyVariable<unsigned int> ("$save_vtk_steps") ;
		maxNumberOfVtkFiles_  = 
			_rbi->getMRubyVariable<unsigned int> ("$number_vtk_saves" );

		numberOfStepsBetweenCheckpointSaves_ = 
			_rbi->getMRubyVariable<unsigned int> ("$numberOfStepsBetweenCheckpointSaves") ;
		maxNumberOfCheckpoints_ = 
			_rbi->getMRubyVariable<unsigned int> ("$maxNumberOfCheckpoints") ;

		numberOfStepsBetweenErrorComputation_ = 
			_rbi->getMRubyVariable<unsigned int>("$error_print_steps") ;

    defaultWallNode_ = 
			buildNodeType (_rbi->getMRubyVariable<std::string> ("$defaultWallNode")) ;
    defaultExternalCornerNode_ = 
			buildNodeType (_rbi->getMRubyVariable<std::string>
												("$defaultExternalCornerNode")) ;
    defaultInternalCornerNode_ =
			buildNodeType (_rbi->getMRubyVariable<std::string>
												("$defaultInternalCornerNode")) ;
    defaultExternalEdgeNode_ =
			buildNodeType (_rbi->getMRubyVariable<std::string>
												("$defaultExternalEdgeNode")) ;
    defaultInternalEdgeNode_ =
			buildNodeType (_rbi->getMRubyVariable<std::string>
												("$defaultInternalEdgeNode")) ;
    defaultNotIdentifiedNode_ =
			buildNodeType (_rbi->getMRubyVariable<std::string>
												("$defaultNotIdentifiedNode")) ;
    defaultExternalEdgePressureNode_ =
			buildNodeType (_rbi->getMRubyVariable<std::string> 
												("$defaultExternalEdgePressureNode")) ;
    defaultExternalCornerPressureNode_ =
			buildNodeType (_rbi->getMRubyVariable<std::string>
												("$defaultExternalCornerPressureNode")) ;
    defaultEdgeToPerpendicularWallNode_ =
			buildNodeType (_rbi->getMRubyVariable<std::string>
												("$defaultEdgeToPerpendicularWallNode")) ;

		vtkDefaultRhoForBB2Nodes_ =
			_rbi->getMRubyVariable<std::string>("$vtkDefaultRhoForBB2Nodes") ;

		if ("mean" != vtkDefaultRhoForBB2Nodes_ &&
				"nan"  != vtkDefaultRhoForBB2Nodes_ )
		{
			stringstream ss ;
			ss << "ERROR: unknown vtkDefaultRhoForBB2Nodes = \"" 
				 << vtkDefaultRhoForBB2Nodes_
				 << "\", available values are \"mean\" \"nan\"\n" ;
			THROW (ss.str()) ;
		}

		setCharacteristicLengthLB( _rbi->getMRubyVariable<double>( "$l_ch_LB"  ) ) ;

		recalculateCoefficients() ;
	}

	ostream & Settings::
	write( ostream & ostr)
	{
		ostr << scientific ;

		ostr << "\nSimulation parameters:\n" ;
		ostr << "lattice              = " << getLatticeArrangementName () << "\n" ;
		ostr << "fluid_model          = " << getFluidModelName         () << "\n" ;
		ostr << "collision_model      = " << getCollisionModelName     () << "\n" ;
		ostr << "data_type            = " << getDataTypeName           () << "\n" ;
		ostr << "computational_engine = " << getComputationalEngineName() << "\n" ;

		ostr << "\nVTK save parameters:\n" ;
		ostr << "save_vtk_steps   = " << getNumberOfStepsBetweenVtkSaves() << "\n" ;
		ostr << "number_vtk_saves = " << getMaxNumberOfVtkFiles() << "\n" ;
		ostr << "\n" ;
		ostr << boolalpha ;
		ostr << "vtk_save_velocity_LB         = " 
				 << shouldSaveVelocityLB        () << "\n" ;
		ostr << "vtk_save_velocity_physical   = " 
				 << shouldSaveVelocityPhysical  () << "\n" ;
		ostr << "vtk_save_rho_LB              = " 
				 << shouldSaveVolumetricMassDensityLB() << "\n" ;
		ostr << "vtk_save_pressure_physical   = "
				 << shouldSavePressurePhysical  () << "\n" ;
		ostr << "vtk_save_nodes               = " 
				 << shouldSaveNodes             () << "\n" ;
		ostr << "vtk_save_mass_flow_fractions = " 
				 << shouldSaveMassFlowFractions () << "\n" ;
		ostr << noboolalpha ;
		ostr << "\n" ;
		ostr << "error_print_steps = " 
				 << getNumberOfStepsBetweenErrorComputation() << "\n" ;

		ostr << "\n" ;
		ostr << "Nx  = " << _Nx   << "\n" ;
		ostr << "Ny  = " << _Ny   << "\n" ;
		ostr << "l_n = " << getCharacteristicLengthLB() << "\n" ;

		ostr << "\n" ;

		ostr << "\n" ;
		ostr << "Computed parameters\n" ;
		ostr << "Physical time step [s]\n" ;
		ostr << "dt_phys   = " << getLatticeTimeStepPhysical() << "\n" ;
		ostr << "Physical lattice unit [m]\n" ;
		ostr << "dx_phys   = " << getLatticeSpacingPhysical()  << "\n" ;
		ostr << "Relaxation factor \n" ;
		ostr << "tau       = " << getTau() << "\n" ;
		ostr << "Kinematic viscosity LB\n" ;
		ostr << "nu_LB     = " << getKinematicViscosityLB()    << "\n" ;
		ostr << "Physical characteristic velocity [m/s]\n" ;
		ostr << "u_ch_phys = " << getCharacteristicVelocityPhysical() << "\n" ;

		ostr << "\n" ;
		ostr << "End simulation condition\n" ;
		ostr << "err = " << getRequiredVelocityRelativeError() << "\n" ;

		ostr << "Initial conditions LB\n" ;
		ostr << "Initial volumetric mass density LB (pressure)\n" ;
		ostr << "rho0_LB = " << getInitialVolumetricMassDensityLB() << "\n" ;
		ostr << "Initial velocity LB along x axis\n" ;
		ostr << "ux0_LB = " << getInitialVelocityLB( Axis::X ) << "\n" ;
		ostr << "Initial velocity LB along y axis\n" ;
		ostr << "uy0_LB = " << getInitialVelocityLB( Axis::Y ) << "\n" ;
		ostr << "\n" ;

		ostr << "Physical simulation parameters\n" ;
		ostr << "Physical kinematic viscosity [m^2/s] \n" ;
		ostr << "nu_phys = " << getKinematicViscosityPhysical() << "\n" ;
		ostr << "Reynolds number\n" ;
		ostr << "Re = " << getReynoldsNumber() << "\n" ;
		ostr << "Physical volumetric mass density (at reference point)\n" ;
		ostr << "rho0_phys = " << getInitialVolumetricMassDensityPhysical() << "\n" ;
		ostr << "\n" ;

		ostr << "Characteristic values\n" ;
		ostr << "Characteristic length LB (channel height LB)\n" ;
		ostr << "l_ch_LB = " << getCharacteristicLengthLB() << "\n" ;
		ostr << "Physical characteristic length [m]\n" ;
		ostr << "l_ch_phys = " << getCharacteristicLengthPhysical() << "\n" ;
		ostr << "Characteristic velocity LB\n" ;
		ostr << "u_ch_LB = " << getCharacteristicVelocityLB() << "\n" ;

		return ostr ;
	}

	NodeType Settings::
	buildNodeType (const std::string name) const
	{
		size_t separatorPosition = name.find(":_:") ;

		std::string baseTypeName, placementModifierName ;

		if (string::npos == separatorPosition)
		{
			baseTypeName = name ;
			placementModifierName = "none" ;
		}
		else
		{
			baseTypeName = name.substr (0, separatorPosition) ;
			placementModifierName = name.substr (separatorPosition+3) ;
		}

		const NodeBaseType nodeBaseType = fromString<NodeBaseType> (baseTypeName) ;
		const PlacementModifier 
			placementModifier = fromString<PlacementModifier> (placementModifierName) ;

		return NodeType (nodeBaseType, placementModifier) ;
	}



	void Settings::
	initialModify (NodeLayout & nodeLayout)
	{
		std::string modificatorScript = 
			readFileContents (getInitialGeometryModificatorPath()) ;

		modificationRhoU_ = 
			_rbi->modifyNodeLayout (nodeLayout, modificatorScript) ;
	}



	void Settings::
	finalModify (NodeLayout & nodeLayout)
	{
		std::string modificatorScript = 
			readFileContents (getFinalGeometryModificatorPath()) ;

		modificationRhoU_ += _rbi->modifyNodeLayout (nodeLayout, modificatorScript) ;
	}



}

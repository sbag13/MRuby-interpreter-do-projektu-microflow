/*
Modified by Szymon Bagiński
baginski.szymon@gmail.com
Wrocław University of Technology
Dec 2017
*/

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <ostream>
#include <cstddef>

#include "RubyInterpreter.hpp"
#include "Axis.hpp"
#include "NodeType.hpp"
#include "NodeLayout.hpp"

namespace microflow
{

/*
	In fact this class should also be template to allow replace double with 
	float. However, it is difficult to concretize this template before 
	parametries file read. Thus, for single precision there will be implicit
	conversion from double.

	TODO: there is some workaround - floating point values may be read as strings
				and converted to numeric values after recognizing data type. This would
				allow for data correctness checks during conversion.

	TODO: change names of attributes at the end of the class definition.
*/
class Settings
{
	public:
		Settings(int a, const std::string simulationDirectoryPath ) ;
		void loadConfiguration1( size_t geometryWidthInCells, 
														size_t geometryHeightInCells, 
														unsigned characteristicLengthInCells ) ;

		Settings( const std::string simulationDirectoryPath ) ;
		Settings() ;
		~Settings() ;

		double getCharacteristicLengthLB() const ;
		double getCharacteristicLengthPhysical() const ;
		double getCharacteristicVelocityLB() const ;
		double getCharacteristicVelocityPhysical() const ;
		double getKinematicViscosityLB() const ;
		double getKinematicViscosityPhysical() const ;
		double getInitialVolumetricMassDensityLB() const ;
		double getInitialVolumetricMassDensityPhysical() const ;
		double getInitialVelocityLB( Axis axis ) const ;
		double getLatticeSpacingPhysical() const ;
		double getLatticeTimeStepPhysical() const ;
		double getTau() const ;
		double getReynoldsNumber() const ;

		// TODO: needed only for tests ?
		void setCharacteristicLengthLB( double lengthLB ) ;
		void setCharacteristicLengthPhysical( double lengthPhys ) ;
		void setCharacteristicVelocityPhysical( double velocityPhys ) ;
		void setKinematicViscosityPhysical( double nuPhys ) ;
		void setInitialVolumetricMassDensityLB( double rhoLB ) ;
		void setInitialVolumetricMassDensityPhysical( double rhoPhys ) ;
		void setInitialVelocityLB( Axis axis, double uLB ) ;
		void setTau( double tau ) ;

		// TODO: private and automatically called when needed ?
		void recalculateCoefficients() ; 

		// TODO: move to separate class ?
		double transformVelocityLBToPhysical( double velocityLB ) const ;
		double transformVelocityPhysicalToLB( double velocityPhysical ) const ;
		double transformVolumetricMassDensityLBToPressurePhysical
							(double volumetricMassDensityLB) const ;
		double transformPressurePhysicalToVolumetricMassDensityLB
							(double pressurePhysical) const ;

		std::string getSimulationDirectoryPath       () const ;
		std::string getSettingsDirectoryPath         () const ;
		std::string getGeometryDirectoryPath         () const ;
		std::string getCheckpointDirectoryPath       () const ;
		std::string getOutputDirectoryPath           () const ;
		std::string getPixelColorDefinitionsFilePath () const ;
		std::string getGeometryPngImagePath          () const ;
		std::string getGeometryVtiImagePath          () const ;
		std::string getInitialGeometryModificatorPath() const ;
		std::string getFinalGeometryModificatorPath  () const ;

		bool isGeometryDefinedByPng() const ;
		bool isGeometryDefinedByVti() const ;

		std::string getLatticeArrangementName () const ;
		std::string getDataTypeName           () const ;
		std::string getFluidModelName         () const ;
		std::string getCollisionModelName     () const ;
		std::string getComputationalEngineName() const ;

		unsigned getZExpandDepth() const ;

		unsigned getNumberOfStepsBetweenVtkSaves() const ;
		unsigned getMaxNumberOfVtkFiles() const ;
		bool shouldSaveVtkInThisStep( unsigned stepNumber ) const ;

		unsigned getNumberOfStepsBetweenCheckpointSaves() const ;
		unsigned getMaxNumberOfCheckpoints() const ;
		bool shouldSaveCheckpointInThisStep( unsigned stepNumber ) const ;

		bool shouldSaveVelocityLB() const ;
		bool shouldSaveVelocityPhysical() const ;
		bool shouldSaveVolumetricMassDensityLB() const ;
		bool shouldSavePressurePhysical() const ;
		bool shouldSaveNodes() const ;
		bool shouldSaveMassFlowFractions() const ;

		unsigned getNumberOfStepsBetweenErrorComputation() const ;
		bool shouldComputeErrorInThisStep( unsigned stepNumber ) const ;

		double getRequiredVelocityRelativeError() const ;

    NodeType getDefaultWallNode() const ;
    NodeType getDefaultExternalCornerNode() const ;
    NodeType getDefaultInternalCornerNode() const ;
    NodeType getDefaultExternalEdgeNode() const ;
    NodeType getDefaultInternalEdgeNode() const ;
    NodeType getDefaultNotIdentifiedNode() const ;
    NodeType getDefaultExternalEdgePressureNode() const ;
    NodeType getDefaultExternalCornerPressureNode() const ;
    NodeType getDefaultEdgeToPerpendicularWallNode() const ;

		// Configuration must be read from ruby script twice, because 
		// at the first time geometry size is not known. 
		// Configuration must be read before geometry read, because
		// in configuration file there is geometry type (ie. D3Q19) which is used
		// during geometry load.
		// Nx and Ny define geometry size, at first run may have any value. 
		//
		// WARNING - settings MUST BE RELOADED in GeometryReader constructor !!!
		//
		void loadConfiguration( size_t geometryWidthInCells, 
														size_t geometryHeightInCells, 
														unsigned characteristicLengthInCells ) ;
		
		
		std::ostream & write( std::ostream & ostr) ;


		//TODO: The below methods probably should be in some other class, but 
		//			geometry modificators are written as Ruby code, which is run
		//			by RubyInterpreter object.
		void initialModify (NodeLayout & nodeLayout) ;
		void finalModify   (NodeLayout & nodeLayout) ;

		// WARNING: modificationRhoU is updated in initialModify() and 
		//					finalModify() methods.
		//          Calling getModificationRhoU() before the above methods
		//					is useless - returns no modifications.
		const ModificationRhoU & getModificationRhoU() const ;


		enum class DefaultValue
		{
			NOT_A_NUMBER, 
			MEAN
		} ;

		DefaultValue getVtkDefaultRhoForBB2Nodes() const ;

		UniversalCoordinates<double> getGeometryOrigin() const ;
		void setGeometryOrigin (UniversalCoordinates<double> origin) ;


	private:
		
		// All paths are relative to below path
		std::string simulationDirectoryPath_ ; 

		/*
			Global parameters for simulation.
		*/
		std::string latticeArrangementName_  ;
		std::string dataTypeName_            ;
		std::string collisionModelName_      ;
		std::string fluidModelName_          ;
		std::string computationalEngineName_ ;

		unsigned zExpandDepth_ ;

		unsigned numberOfStepsBetweenVtkSaves_ ;
		unsigned maxNumberOfVtkFiles_ ;

		unsigned numberOfStepsBetweenCheckpointSaves_ ;
		unsigned maxNumberOfCheckpoints_ ;

		bool shouldSaveVelocityLB_ ;
		bool shouldSaveVelocityPhysical_ ;
		bool shouldSaveVolumetricMassDensityLB_ ;
		bool shouldSavePressurePhysical_ ;
		bool shouldSaveNodes_ ;
		bool shouldSaveMassFlowFractions_ ;
		
		unsigned numberOfStepsBetweenErrorComputation_ ; 

		unsigned _Nx ;
		unsigned _Ny ;

    NodeType defaultWallNode_                    ;
    NodeType defaultExternalCornerNode_          ;
    NodeType defaultInternalCornerNode_          ;
    NodeType defaultExternalEdgeNode_            ;
    NodeType defaultInternalEdgeNode_            ;
    NodeType defaultNotIdentifiedNode_           ;
    NodeType defaultExternalEdgePressureNode_    ;
    NodeType defaultExternalCornerPressureNode_  ;
    NodeType defaultEdgeToPerpendicularWallNode_ ;

		NodeType buildNodeType (const std::string name) const ;


		double requiredVelocityRelativeError_ ;
		double initialVolumetricMassDensityLB_ ;
		double initialVelocityLB_ [ 3 ] ;
		double kinematicViscosityPhysical_ ; // [m^2/s]
		double reynoldsNumber_ ;
		double initialVolumetricMassDensityPhysical_ ;
		double characteristicLengthLB_ ;
		double characteristicLengthPhysical_   ; // [m]
		double characteristicVelocityPhysical_ ; // [m/s]


		std::string vtkDefaultRhoForBB2Nodes_ ;


		double kinematicViscosityLB_ ;
		double latticeTimeStepPhysical_ ;
		double latticeSpacingPhysical_ ;
		double tau_ ;
		double characteristicVelocityLB_ ;


		std::unique_ptr<MRubyInterpreter> _rbi = nullptr ; 

		ModificationRhoU modificationRhoU_ ;

		UniversalCoordinates<double> geometryOrigin_ ;
} ;



}



#include "Settings.hh"



#endif

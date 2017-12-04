#include "gtest/gtest.h"

#include "RubyInterpreter.hpp"
#include "NodeLayoutTest.hpp"

using namespace microflow ;

TEST (MRubyInterpreter, constructor_destructor)
{
	EXPECT_NO_THROW( MRubyInterpreter::getMRubyInterpreter() ; ) ;
}

TEST (MRubyInterpreter, constructor_destructor_twice)
{
	EXPECT_NO_THROW( MRubyInterpreter::getMRubyInterpreter() ; ) ;
	EXPECT_NO_THROW( MRubyInterpreter::getMRubyInterpreter() ; ) ;
}

TEST(MRubyInterpreter, single)
{
	std::unique_ptr<MRubyInterpreter> ri = nullptr ;

	EXPECT_NO_THROW( ri = MRubyInterpreter::getMRubyInterpreter() ; ) ;

	EXPECT_NO_THROW( ri->runScript("$a = 59") ; ) ;
	EXPECT_EQ(59u, ri->getMRubyVariable<unsigned int>("$a") ) ;
	EXPECT_NO_THROW( ri->runScript("$a = 100") ; ) ;
	EXPECT_EQ(100u, ri->getMRubyVariable<unsigned int>("$a") ) ;
	EXPECT_NO_THROW( ri->runScript("$a = 0") ; ) ;
	EXPECT_EQ(0u, ri->getMRubyVariable<unsigned int>("$a") ) ;
	EXPECT_NO_THROW( ri->runScript("$a += 1") ; ) ;
	EXPECT_EQ(1u, ri->getMRubyVariable<unsigned int>("$a") ) ;
}

TEST(MRubyInterpreter, INT_MAX_conversion)
{
	std::unique_ptr<MRubyInterpreter> ri = nullptr ;

	EXPECT_NO_THROW( ri = MRubyInterpreter::getMRubyInterpreter() ; ) ;

	std::stringstream ss ; 					
	ss << "$a = " << INT_MAX << "\n" ;		// INT_MAX is max integer value in mruby
	EXPECT_NO_THROW( ri->runScript(ss.str().c_str()) ) ;
	EXPECT_EQ(INT_MAX, ri->getMRubyVariable<int>("$a") ) ;
}

TEST(MRubyInterpreter, call_getMRubyVariable_string_with_valid_arguments__get_valid_string)
{
	std::unique_ptr<MRubyInterpreter> ri = nullptr ;

	EXPECT_NO_THROW( ri = MRubyInterpreter::getMRubyInterpreter() ; ) ;
	EXPECT_NO_THROW( ri->runScript("$a = String.new(\"test string\")") ; ) ;
	EXPECT_EQ("test string", ri->getMRubyVariable<std::string>("$a") ) ;
}

TEST(MRubyInterpreter, DISABLED_parallel)	//niepotrzebne przy wielu interpreterach, 
{
	std::unique_ptr<MRubyInterpreter> ri1 = nullptr, ri2 = nullptr ;

	EXPECT_NO_THROW( ri1 = MRubyInterpreter::getMRubyInterpreter() ; ) ;
	EXPECT_NO_THROW( ri2 = MRubyInterpreter::getMRubyInterpreter() ; ) ;

	EXPECT_NO_THROW( ri1->runScript("$a = 59") ; ) ;
	EXPECT_NO_THROW( ri2->runScript("$a = 9") ; ) ;
	EXPECT_EQ(59u, ri1->getMRubyVariable<unsigned int>("$a") ) ;
	EXPECT_EQ(9u, ri2->getMRubyVariable<unsigned int>("$a") ) ;
	EXPECT_NO_THROW( ri1->runScript("$a = 100") ; ) ;
	EXPECT_EQ(100u, ri1->getMRubyVariable<unsigned int>("$a") ) ;
	EXPECT_EQ(9u, ri2->getMRubyVariable<unsigned int>("$a") ) ;
	EXPECT_NO_THROW( ri2->runScript("$a = 10") ; ) ;
	EXPECT_NO_THROW( ri1->runScript("$a = 0") ; ) ;
	EXPECT_EQ(0u, ri1->getMRubyVariable<unsigned int>("$a") ) ;
	EXPECT_EQ(10u, ri2->getMRubyVariable<unsigned int>("$a") ) ;
	EXPECT_NO_THROW( ri1->runScript("$a += 1") ; ) ;
	EXPECT_EQ(1u, ri1->getMRubyVariable<unsigned int>("$a") ) ;

	std::stringstream ss ; 
	ss << "$a = " << INT_MAX << "\n" ;			//bylo UINT_MAX
	EXPECT_NO_THROW( ri1->runScript(ss.str().c_str()) ) ;
	EXPECT_EQ(INT_MAX, ri1->getMRubyVariable<unsigned int>("$a") ) ;

	EXPECT_EQ(INT_MAX, ri1->getMRubyVariable<unsigned int>("$a") ) ;
}

TEST (MRubyInterpreter, modifyNodeLayout)
{
	std::unique_ptr<MRubyInterpreter> ri = nullptr ;

	EXPECT_NO_THROW( ri = MRubyInterpreter::getMRubyInterpreter() ; ) ;

	NodeLayout nodeLayout = createSolidNodeLayout (4,4,4) ;

	std::string rubyCode = 
		"puts getNode(1,1,1);    "
		"setNodes( coordinates(1,1,1), :baseType => fluid) ; "
		"puts getNode(1,1,1);    "
		"puts getNode(0,0,0);    "
		"puts getNode(10,10,10); "

		"puts getSize() ;        "
		;

	ri->modifyNodeLayout (nodeLayout, rubyCode) ;

	EXPECT_EQ (nodeLayout.getNodeType(1,1,1), NodeBaseType::FLUID) ;
}

TEST (MRubyInterpreter, modifyNodeLayout_setNode_variants)
{
	std::unique_ptr<MRubyInterpreter> ri = nullptr ;

	EXPECT_NO_THROW( ri = MRubyInterpreter::getMRubyInterpreter() ; ) ;

	NodeLayout nodeLayout = createSolidNodeLayout (4,4,4) ;


	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getBaseType(), NodeBaseType::SOLID) ;
	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getPlacementModifier(), PlacementModifier::NONE) ;

	ri->modifyNodeLayout (nodeLayout, 
		"setNodes( coordinates( 1,1,1 ), :baseType => fluid, :placementModifier => top) ; puts getNode(1,1,1);") ;

	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getBaseType(), NodeBaseType::FLUID) ;
	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getPlacementModifier(), PlacementModifier::TOP) ;

	ri->modifyNodeLayout (nodeLayout, 
		"setNodes( coordinates( 1,1,1 ), :baseType => velocity) ; ") ;

	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getBaseType(), NodeBaseType::VELOCITY) ;
	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getPlacementModifier(), PlacementModifier::TOP) ;

	ri->modifyNodeLayout (nodeLayout, 
		"setNodes( coordinates( 1,1,1 ), :placementModifier => bottom) ; ") ;

	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getBaseType(), NodeBaseType::VELOCITY) ;
	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getPlacementModifier(), PlacementModifier::BOTTOM) ;

	auto modificationsRhoU = ri->modifyNodeLayout (nodeLayout, 
		"setNodes( coordinates( 1,1,1 ), :rhoPhysical => 0.5) ; ") ;

	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getBaseType(), NodeBaseType::VELOCITY) ;
	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getPlacementModifier(), PlacementModifier::BOTTOM) ;
	ASSERT_EQ (modificationsRhoU.rhoPhysical.size(), 1u) ;
	EXPECT_EQ (modificationsRhoU.uPhysical.size(), 0u) ;
	EXPECT_EQ (modificationsRhoU.rhoBoundaryPhysical.size(), 0u) ;
	EXPECT_EQ (modificationsRhoU.uBoundaryPhysical.size(), 0u) ;
	EXPECT_EQ (modificationsRhoU.rhoPhysical[0].coordinates, Coordinates(1,1,1) ) ;
	EXPECT_EQ (modificationsRhoU.rhoPhysical[0].value, 0.5 ) ;

	modificationsRhoU = ri->modifyNodeLayout (nodeLayout, 
		"setNodes( coordinates( 2,2,2 ), :rhoBoundaryPhysical => 0.25) ; ") ;

	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getBaseType(), NodeBaseType::VELOCITY) ;
	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getPlacementModifier(), PlacementModifier::BOTTOM) ;
	EXPECT_EQ (modificationsRhoU.rhoPhysical.size(), 0u) ;
	EXPECT_EQ (modificationsRhoU.uPhysical.size(), 0u) ;
	ASSERT_EQ (modificationsRhoU.rhoBoundaryPhysical.size(), 1u) ;
	EXPECT_EQ (modificationsRhoU.uBoundaryPhysical.size(), 0u) ;
	EXPECT_EQ (modificationsRhoU.rhoBoundaryPhysical[0].coordinates, Coordinates(2,2,2) ) ;
	EXPECT_EQ (modificationsRhoU.rhoBoundaryPhysical[0].value, 0.25 ) ;

	modificationsRhoU = ri->modifyNodeLayout (nodeLayout, 
		"setNodes( coordinates( 2,2,2 ), :uPhysical => [1.5, 2.5, 3.5] ) ; ") ;

	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getBaseType(), NodeBaseType::VELOCITY) ;
	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getPlacementModifier(), PlacementModifier::BOTTOM) ;
	EXPECT_EQ (modificationsRhoU.rhoPhysical.size(), 0u) ;
	ASSERT_EQ (modificationsRhoU.uPhysical.size(), 1u) ;
	EXPECT_EQ (modificationsRhoU.rhoBoundaryPhysical.size(), 0u) ;
	EXPECT_EQ (modificationsRhoU.uBoundaryPhysical.size(), 0u) ;
	EXPECT_EQ (modificationsRhoU.uPhysical[0].coordinates, Coordinates(2,2,2) ) ;
	EXPECT_EQ (modificationsRhoU.uPhysical[0].value[0], 1.5 ) ;
	EXPECT_EQ (modificationsRhoU.uPhysical[0].value[1], 2.5 ) ;
	EXPECT_EQ (modificationsRhoU.uPhysical[0].value[2], 3.5 ) ;

	modificationsRhoU = ri->modifyNodeLayout (nodeLayout, 
		"setNodes( coordinates( 1,2,3 ), :uBoundaryPhysical => [10.0, 11.0, 12.0] ) ; ") ;

	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getBaseType(), NodeBaseType::VELOCITY) ;
	EXPECT_EQ (nodeLayout.getNodeType(1,1,1).getPlacementModifier(), PlacementModifier::BOTTOM) ;
	EXPECT_EQ (modificationsRhoU.rhoPhysical.size(), 0u) ;
	EXPECT_EQ (modificationsRhoU.uPhysical.size(), 0u) ;
	EXPECT_EQ (modificationsRhoU.rhoBoundaryPhysical.size(), 0u) ;
	ASSERT_EQ (modificationsRhoU.uBoundaryPhysical.size(), 1u) ;
	EXPECT_EQ (modificationsRhoU.uBoundaryPhysical[0].coordinates, Coordinates(1,2,3) ) ;
	EXPECT_EQ (modificationsRhoU.uBoundaryPhysical[0].value[0], 10 ) ;
	EXPECT_EQ (modificationsRhoU.uBoundaryPhysical[0].value[1], 11 ) ;
	EXPECT_EQ (modificationsRhoU.uBoundaryPhysical[0].value[2], 12 ) ;
}

#include "RipperEngine.hpp"

namespace SimpleRipper
{
  RipperEngine::RipperEngine
    (
    CDDrive & aDrive	//!< CD Drive to rip tracks from
    )
  : mDrive( aDrive )
  , mType( aDrive.getCDType() )
  {
  }
  
  RipperEngine::~RipperEngine()
  {
  }
   
} // end namespace SimpleRipper
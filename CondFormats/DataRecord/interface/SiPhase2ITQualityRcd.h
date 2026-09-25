#ifndef CondFormats_SiPhase2ITQualityRcd_h
#define CondFormats_SiPhase2ITQualityRcd_h

#include "FWCore/Framework/interface/EventSetupRecordImplementation.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "FWCore/Utilities/interface/mplVector.h"


class SiPhase2ITQualityRcd : public edm::eventsetup::DependentRecordImplementation<
                              SiPhase2ITQualityRcd, edm::mpl::Vector<TrackerDigiGeometryRecord> > {};
#endif

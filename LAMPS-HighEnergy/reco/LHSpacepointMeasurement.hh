#ifndef LHSPACEPOINTMEASUREMENT_HH
#define LHSPACEPOINTMEASUREMENT_HH

#include "AbsMeasurement.h"
#include "SpacepointMeasurement.h"
#include "TrackCandHit.h"

#include "KBHit.hh"

namespace genfit {
  //////////////////////////////////////////////////////////////////////////////////

  class LHSpacepointMeasurement : public SpacepointMeasurement
  {
    public:
      LHSpacepointMeasurement();
      LHSpacepointMeasurement(const KBHit* detHit, const TrackCandHit* hit);

      virtual LHSpacepointMeasurement* clone() const { return new LHSpacepointMeasurement(*this); }

    ClassDef(LHSpacepointMeasurement, 1)
  };

  //////////////////////////////////////////////////////////////////////////////////
}
#endif

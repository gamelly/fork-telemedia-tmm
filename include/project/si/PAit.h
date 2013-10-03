/*
 * PAit.h
 *
 *  Created on: 22/03/2013
 *      Author: Felippe Nagato
 */

#ifndef PAIT_H_
#define PAIT_H_

#include "project/ProjectInfo.h"
#include "si/Ait.h"
#include <iostream>

using namespace std;
using namespace br::pucrio::telemidia::mpeg2;

namespace br {
namespace pucrio {
namespace telemidia {
namespace tool {

class PAit : public ProjectInfo, public Ait {

	private:

	protected:
		ProjectInfo* carouselProj;
	public:
		PAit();
		virtual ~PAit();

		void setCarouselProj(ProjectInfo* proj);
		ProjectInfo* getCarouselProj();

};

}
}
}
}


#endif /* PAIT_H_ */

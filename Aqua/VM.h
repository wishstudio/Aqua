#include "VMTypes.h"

struct Frame
{
	reg *registerBase;
	Method *method;
	uint32 nextpc;
	uint32 nextExceptionClause; /* Next exception clause to handle */
};

extern Frame *currentFrame;
extern reg *currentBase;

void runMethod(Frame *startFrame, Method *startMethod, reg *base);

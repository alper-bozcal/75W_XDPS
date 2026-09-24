#include "../DynamicPassword/dynamic_pass.h"

typedef enum{
	DYNAMIC_PASS_USER = 0,
	DYNAMIC_PASS_SERVICE,
    DYNAMIC_PASS_FACTORY,
	DYNAMIC_PASS_ENKO,
	DYNAMIC_PASS_END,
}DYNAMIC_PASS_TYPE_e;

void setDynamicPasswordSetup(DynamicPass_t *DynamicPassObj);

DynamicPass_t *getDynamicPasswordObj();

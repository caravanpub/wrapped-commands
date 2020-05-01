#ifndef RULEDEFS_H
#define RULEDEFS_H
#include "list.h"
list* rm_allowed();
list* rm_forbidden();
list* cp_allowed();
list* cp_forbidden();
list* mkdir_allowed();
list* mkdir_forbidden();
list* chmod_allowed();
list* chmod_forbidden();
list* chown_allowed();
list* chown_forbidden();
list* chgrp_allowed();
list* chgrp_forbidden();
list* cat_allowed();
list* cat_forbidden();
list* rmdir_allowed();
list* rmdir_forbidden();
#endif


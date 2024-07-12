#ifndef LABELS_CONVERSION_H
#define LABELS_CONVERSION_H
#include "p4_api.h"

int updateTags(P4API* p4, const std::string& depotPath, git_repository* repo);

#endif // LABELS_CONVERSION_H

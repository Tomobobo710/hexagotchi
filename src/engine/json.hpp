// Header-only include for nlohmann/json
// This is a wrapper that includes the external json.hpp from the rres directory.
// The actual nlohmann/json single-header file should be placed at:
//   rres/src/external/json.hpp
//
// For now, we include from the local src/external directory where we downloaded it.

#include "../external/json.hpp"
using json = nlohmann::json;

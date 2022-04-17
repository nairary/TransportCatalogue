#include "json_reader.h"
#include "map_renderer.h"

#include <cassert>
#include <chrono>
#include <sstream>
#include <memory>
#include <string_view>

int main() {
    JsonReader jr(std::cin);
    transport_catalogue::TransportCatalogue transport_catalogue_;
    jr.AddToCatalogue(transport_catalogue_);
    maper::Maper svg_map(jr.AddRenderOptions());
    json::Document stat_request = jr.ParseRequest(transport_catalogue_, svg_map);
    Print(stat_request,std::cout);
}

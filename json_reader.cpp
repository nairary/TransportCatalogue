#include <stdexcept>
#include <string>
#include <vector>
#include <string_view>
#include <algorithm>
#include "json_reader.h"
#include "svg.h"

using namespace std::literals::string_literals;

JsonReader::JsonReader(std::istream &input) {
    json_requests_ = std::move(std::make_unique<json::Node>(json::Load(input).GetRoot()));
}

void AddingBuses(transport_catalogue::TransportCatalogue& transport_catalogue_,
                 std::vector<const json::Node*>& buses_to_push) {
        for (const auto* bus_to_push : buses_to_push) {
            auto& bus_ = bus_to_push->AsMap();
            std::vector<std::string_view> stop_names;
            for (auto& stop : bus_.at("stops").AsArray()) {
                stop_names.push_back(stop.AsString());
            }
            std::vector<const transport_catalogue::Stop*> stops;
            stops.reserve(stop_names.size());

            for (auto& stop_name : stop_names) {
                auto temp_stop = transport_catalogue_.GetStop(stop_name);
                if (temp_stop) {
                    stops.push_back(temp_stop);
                } else {
                    throw std::invalid_argument("unknown stop_name: " + static_cast<std::string>(stop_name));
                }
            }

            bool round_flag = bus_to_push->AsMap().at("is_roundtrip").AsBool();
            std::string bus_name = bus_.at("name").AsString();
            transport_catalogue_.AddBus({bus_name, stop_names, round_flag});
        }
}

void JsonReader::AddToCatalogue(transport_catalogue::TransportCatalogue &transport_catalogue_) {
    //array of "base requests"
    auto& base_requests_array = json_requests_->AsMap().at("base_requests").AsArray();
    //container for temp buses to push after all stops
    std::vector<const json::Node*> buses_to_push;
    //container fo temp distances to push after all stops are added
    std::map<std::string, json::Dict> distances_to_push;

    for (const auto& base_request : base_requests_array) {
        auto& request = base_request.AsMap();
        if (request.at("type").AsString() == "Stop") {
            std::string stop_name = request.at("name").AsString();
            geo::Coordinates coordinates {};
            coordinates.lat = request.at("latitude").AsDouble();
            coordinates.lng = request.at("longitude").AsDouble();
            transport_catalogue_.AddStop(stop_name, coordinates.lat, coordinates.lng);
            distances_to_push[stop_name] = request.at("road_distances").AsMap();
        } else {
            buses_to_push.push_back(&base_request);
        }
    }
    //adding distances for stops
    for (std::pair<const std::string, json::Dict>& distance : distances_to_push) {
        for (std::pair<std::string, json::Node> distanceTO : distance.second) {
            transport_catalogue_.SetDistanceBetweenStops(distance.first,
                                                         distanceTO.first, distanceTO.second.AsInt());
        }
    }
    //adding buses to catalogue
    AddingBuses(transport_catalogue_, buses_to_push);
}

svg::Color MakeColor(const json::Node& node_color) {
    if (node_color.IsString()) {
        return svg::Color{node_color.AsString()};
    } else if (node_color.IsArray()) {
        const json::Array& color_array = node_color.AsArray();
        if (color_array.size() > 4 || color_array.size() < 3)
            throw std::logic_error("");
        uint8_t r = color_array[0].AsInt();
        uint8_t g = color_array[1].AsInt();
        uint8_t b = color_array[2].AsInt();
        if (color_array.size() == 3) {
        return svg::Color{svg::Rgb{r,g,b}};
        } else {
            return svg::Color{svg::Rgba{r, g, b, color_array[3].AsDouble()}};
        }
    }
    throw std::invalid_argument("");
}

maper::RenderOptions JsonReader::AddRenderOptions() {
    auto& render_settings_map = json_requests_->AsMap().at("render_settings").AsMap();
    maper::RenderOptions render_options;
    render_options.width = render_settings_map.at("width").AsDouble();
    render_options.height = render_settings_map.at("height").AsDouble();
    render_options.padding = render_settings_map.at("padding").AsDouble();
    render_options.line_width = render_settings_map.at("line_width").AsDouble();
    render_options.stop_radius = render_settings_map.at("stop_radius").AsDouble();
    render_options.bus_label_font_size = render_settings_map.at("bus_label_font_size").AsInt();
    render_options.bus_label_offset.x = render_settings_map.at("bus_label_offset").AsArray()[0].AsDouble();
    render_options.bus_label_offset.y = render_settings_map.at("bus_label_offset").AsArray()[1].AsDouble();
    render_options.stop_label_offset.x = render_settings_map.at("stop_label_offset").AsArray()[0].AsDouble();
    render_options.stop_label_offset.y = render_settings_map.at("stop_label_offset").AsArray()[1].AsDouble();
    render_options.stop_label_font_size = render_settings_map.at("stop_label_font_size").AsInt();
    render_options.underlayer_width = render_settings_map.at("underlayer_width").AsDouble();
    render_options.underlayer_color = MakeColor(render_settings_map.at("underlayer_color"));
    for (const json::Node& current_color : render_settings_map.at("color_palette").AsArray()) {
        render_options.color_palette.push_back(MakeColor(current_color));
    }
    return render_options;
}

json::Document JsonReader::ParseRequest(transport_catalogue::TransportCatalogue& transport_catalogue_,
                                        std::optional<maper::Maper> maper_) {
    auto stat_requests_array = json_requests_->AsMap().at("stat_requests").AsArray();
    json::Array json_requests_result;
    for (const auto& stat_request : stat_requests_array) {
        auto stat_request_map = stat_request.AsMap();
        int request_id = stat_request_map.at("id").AsInt();
        json::Dict request_result;
        request_result.insert({ "request_id", json::Node(request_id) });

        if (stat_request_map.at("type").AsString() == "Stop") {
            const std::string& stop_name = stat_request_map.at("name").AsString();
            json::Array buses_array;
            if (transport_catalogue_.GetStop(stop_name)) {
                auto buses = transport_catalogue_.GetBusesByStop(stop_name);
                if (!buses.empty()) {
                    for (const transport_catalogue::Bus* bus_name: buses) {
                        buses_array.push_back(json::Node{static_cast<std::string>(bus_name->number)});
                    }
                    std::sort(buses_array.begin(), buses_array.end(),
                              [](const json::Node &a, const json::Node &b) { return a.AsString() < b.AsString(); });
                }
                request_result.insert({"buses", buses_array});
            } else {
                request_result.insert({"error_message", json::Node("not found"s)});
            }
        } else if (stat_request_map.at("type").AsString() == "Bus") {
            if (!transport_catalogue_.GetBus(stat_request_map.at("name").AsString())) {
                request_result.insert({"error_message", json::Node("not found"s)});
            } else {
                const std::string &bus_name = stat_request_map.at("name").AsString();
                auto bus = transport_catalogue_.GetBus(bus_name);
                request_result.insert({"curvature", json::Node(bus->curvature)});
                request_result.insert({"stop_count", json::Node(static_cast<int>(bus->all_stops_count))});
                request_result.insert({"unique_stop_count", json::Node(static_cast<int>(bus->unique_stops_count))});
                request_result.insert({"route_length", json::Node(bus->route_length)});
            }
        }
        else if (stat_request_map.at("type").AsString() == "Map") {
            svg::Document svg = maper_->MakeMap(transport_catalogue_);
            std::ostringstream out;
            svg.Render(out);
            request_result.insert({"map", json::Node(out.str())});
        }
        json_requests_result.push_back(request_result);
    }
    return json::Document(json::Node(std::move(json_requests_result)));

}

#include "transport_catalogue.h"

namespace transport_catalogue {

    void TransportCatalogue::AddBus(Bus bus) {
        if (GetBus(bus.number))
            return;
        std::unordered_set<std::string_view> unique;
        double geo_distance = 0;
        double route_length = 0;
        auto values = bus.stops_name;
        bus.stops_name = {};
        if (bus.is_round_trip_) {
            auto prev = GetStop(values[0]);
            for (size_t i = 0; i < values.size(); ++i) {
                if (i == 0) {
                    unique.insert(values[i]);
                    bus.stops.push_back(GetStop(values[i]));
                    continue;
                }
                auto curr = GetStop(values[i]);
                geo_distance += ComputeDistance(prev->coordinates, curr->coordinates);
                route_length += GetDistanceBetweenStops(prev->name, curr->name);
                unique.insert(values[i]);
                bus.stops.push_back(GetStop(values[i]));
                prev = curr;
            }
            bus.all_stops_count = values.size() ;
        } else {
            auto prev = GetStop(values[0]);
            for (size_t i = 0; i < values.size(); ++i) {
                if (i == 0) {
                    unique.insert(values[i]);
                    bus.stops.push_back(GetStop(values[i]));
                    continue;
                }
                auto curr = GetStop(values[i]);
                geo_distance += ComputeDistance(prev->coordinates, curr->coordinates) * 2;
                route_length += GetDistanceBetweenStops(prev->name, curr->name) +
                                GetDistanceBetweenStops(curr->name, prev->name);
                unique.insert(values[i]);
                bus.stops.push_back(GetStop(values[i]));
                prev = curr;
            }
            bus.all_stops_count = (values.size() * 2) - 1;
        }
        bus.unique_stops_count = unique.size();
        bus.route_length = route_length;
        bus.curvature = bus.route_length / geo_distance;
        const auto &enter_bus = buses_.emplace_back(std::move(bus));
        bus_name_to_bus_[enter_bus.number] = &enter_bus;
        for (const auto &stop: enter_bus.stops) {
            stops_to_bus_[stop].insert(GetBus(enter_bus.number));
        }
    }

    void TransportCatalogue::AddStop(std::string_view stop_name, double lat, double lng) {
        geo::Coordinates coordinates{lat, lng};
        Stop stop{std::string(stop_name), coordinates};
        const auto &enter_stop = stops_.emplace_back(std::move(stop));
        stop_name_to_stop_[enter_stop.name] = &enter_stop;
        stops_to_bus_[&enter_stop];
    }
    const Bus *TransportCatalogue::GetBus(std::string_view bus_number) const {
        if (!bus_name_to_bus_.count(bus_number)) {
            return nullptr;
        }
        return bus_name_to_bus_.at(bus_number);
    }
    const Stop *TransportCatalogue::GetStop(std::string_view stop_name) const {
        if (!stop_name_to_stop_.count(stop_name)) {
            return nullptr;
        }
        return stop_name_to_stop_.at(stop_name);
    }
    void TransportCatalogue::SetDistanceBetweenStops(std::string_view stop_name, std::string_view next_stop_name,
                                                     double distance) {
        stop_pair_to_distance_.insert_or_assign(std::make_pair(GetStop(stop_name), GetStop(next_stop_name)), distance);
    }
    double
    TransportCatalogue::GetDistanceBetweenStops(std::string_view stop_name, std::string_view next_stop_name) const {
        auto stops = std::make_pair(GetStop(stop_name), GetStop(next_stop_name));
        double result;
        if (stop_pair_to_distance_.count(stops)) {
            result = stop_pair_to_distance_.at(stops);
        } else {
            result = stop_pair_to_distance_.at(std::make_pair(GetStop(next_stop_name), GetStop(stop_name)));
        }
        return result;
    }

    std::unordered_set<const Bus*> TransportCatalogue::GetBusesByStop(std::string_view stop_name) {
        return stops_to_bus_.at(GetStop(stop_name));
    }

    const std::vector<const Bus*> TransportCatalogue::GetAllBuses() const {
        std::vector<const Bus*> buses;
        for (auto& bus : bus_name_to_bus_) {
            buses.push_back(bus.second);
        }
        return buses;
    }
    const std::vector<const Stop *> TransportCatalogue::GetAllStops() const {
        std::vector<const Stop*> stops;
        for (auto& stop : stop_name_to_stop_) {
            stops.push_back(stop.second);
        }
        return stops;
    }

    bool TransportCatalogue::HasBuses(const Stop * stop) const {
        if (this->stops_to_bus_.count(stop)) {
            if (this->stops_to_bus_.at(stop).size() > 0) {
                return true;
            }
        }
        return false;
    }

}
#pragma once

#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <set>
#include "geo.h"

namespace transport_catalogue {
    struct Stop {
        std::string name;
        geo::Coordinates coordinates;
    };
    struct Bus {
        Bus (std::string name, std::vector<std::string_view> stops, bool f)
                :number(name),
                 stops_name(stops),
                 is_round_trip_(f){
        }
        std::vector<const Stop*> stops;
        std::string number;
        double route_length = 0;
        double curvature = 0;
        size_t unique_stops_count = 0;
        size_t all_stops_count = 0;
        std::vector<std::string_view> stops_name;
        bool is_round_trip_ = false;
    };
    class TransportCatalogue {
    public:
        struct StopsHasher {
            size_t operator()(const std::pair<const Stop *, const Stop *> &two_stops) const {
                size_t h_1 = hasher_(two_stops.first);
                size_t h_2 = hasher_(two_stops.second);
                return h_2 * 12 + h_1 * (12 * 12);
            }
        private:
            std::hash<const void *> hasher_;
        };

        void AddBus(Bus bus);

        void AddStop(std::string_view stop_name, double lat, double lng);

        const Bus *GetBus(std::string_view bus_number) const;

        const std::vector<const Bus*> GetAllBuses() const;

        const std::vector<const Stop*> GetAllStops() const;

        const Stop *GetStop(std::string_view stop_name) const;

        void SetDistanceBetweenStops(std::string_view stop_from, std::string_view stop_to, double distance);

        double GetDistanceBetweenStops(std::string_view stop_from, std::string_view stop_to) const;

        bool HasBuses(const Stop* stop) const;

        std::unordered_set<const Bus*> GetBusesByStop(std::string_view stop_name);

    private:
        std::deque<Bus> buses_;
        std::deque<Stop> stops_;
        std::unordered_map<std::string_view, const Bus *> bus_name_to_bus_;
        std::unordered_map<std::string_view, const Stop *> stop_name_to_stop_;
        std::unordered_map<const Stop *, std::unordered_set<const Bus*>> stops_to_bus_;
        std::unordered_map<const std::pair<const Stop *, const Stop *>, double, StopsHasher> stop_pair_to_distance_;
    };
}
#pragma once

#include <istream>
#include <chrono>

#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

class JsonReader {
public:
    JsonReader(std::istream& input);

    json::Document ParseRequest(transport_catalogue::TransportCatalogue& transport_catalogue_,
                                std::optional<maper::Maper> maper_);

    void AddToCatalogue (transport_catalogue::TransportCatalogue& transport_catalogue);

    maper::RenderOptions AddRenderOptions();

private:
    std::unique_ptr<json::Node> json_requests_;
};
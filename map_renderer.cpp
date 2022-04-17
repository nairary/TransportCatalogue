#include "map_renderer.h"

bool maper::IsZero(double value) {
    return std::abs(value) < maper::EPSILON;
}

void maper::Maper::MakeSphereProjector(const std::vector<geo::Coordinates> &points) {
    projector_ = SphereProjector{ points.begin(), points.end(),
                                  options_.width, options_.height, options_.padding};
}

void maper::Maper::AddRouteLines(svg::Document &doc, const transport_catalogue::Bus *bus, size_t color_number) {
        svg::Polyline polyline;
        for (const transport_catalogue::Stop* stop : bus->stops) {
            polyline.AddPoint(projector_(stop->coordinates));
        }
        if (!bus->is_round_trip_) {
            for (int i = bus->stops.size() - 2; i > 0 ; --i) {
                polyline.AddPoint(projector_(bus->stops[i]->coordinates));
            }
            polyline.AddPoint(projector_(bus->stops[0]->coordinates));
        }
        size_t color = color_number % options_.color_palette.size();

    polyline.SetFillColor(svg::NoneColor)
            .SetStrokeColor(options_.color_palette[color])
            .SetStrokeWidth(options_.line_width)
            .SetStrokeLineCap( svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc.Add(polyline);
}

void maper::Maper::AddRouteNames(svg::Document &doc, const transport_catalogue::Bus *bus, size_t color_number) {
        svg::Text text;
        svg::Text text_background;
        size_t color = color_number % options_.color_palette.size();
        //устанавливаемы отправную точку маршрута
    text_background.SetPosition(projector_({bus->stops[0]->coordinates}))
            .SetOffset({options_.bus_label_offset})
            .SetFontSize(options_.bus_label_font_size).SetFontFamily("Verdana").SetFontWeight("bold").SetData(bus->number)
            .SetFillColor(options_.underlayer_color).SetStrokeColor(options_.underlayer_color)
            .SetStrokeWidth(options_.underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    text.SetPosition(projector_({bus->stops[0]->coordinates}))
            .SetOffset({ options_.bus_label_offset})
            .SetFontSize(options_.bus_label_font_size).SetFontFamily("Verdana").SetFontWeight("bold").SetData(bus->number)
            .SetFillColor(options_.color_palette[color]);

    doc.Add(text_background);
    doc.Add(text);
        //устанавливаем конечную точку маршрута
        if (!bus->is_round_trip_ && bus->stops[0] != bus->stops.back()) {
            text_background.SetPosition(projector_({bus->stops.back()->coordinates}))
                    .SetOffset({options_.bus_label_offset})
                    .SetFontSize(options_.bus_label_font_size).SetFontFamily("Verdana").SetFontWeight("bold").SetData(
                            bus->number)
                    .SetFillColor(options_.underlayer_color).SetStrokeColor(options_.underlayer_color)
                    .SetStrokeWidth(options_.underlayer_width).SetStrokeLineCap(
                            svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            text.SetPosition(projector_({bus->stops.back()->coordinates}))
                    .SetOffset({options_.bus_label_offset})
                    .SetFontSize(options_.bus_label_font_size).SetFontFamily("Verdana").SetFontWeight("bold").SetData(
                            bus->number)
                    .SetFillColor(options_.color_palette[color]);

            doc.Add(text_background);
            doc.Add(text);
        }
}

void maper::Maper::AddStopPoint(svg::Document &doc, const transport_catalogue::Stop *stop) {
    svg::Circle stop_point;
    stop_point.SetCenter(projector_({stop->coordinates}))
        .SetRadius(options_.stop_radius)
        .SetFillColor("white");
    doc.Add(stop_point);
}

void maper::Maper::AddStopNames(svg::Document &doc, const transport_catalogue::Stop *stop) {
    svg::Text text;
    svg::Text text_background;
    text_background.SetPosition(projector_({stop->coordinates}))
            .SetOffset({options_.stop_label_offset})
            .SetFontSize(options_.stop_label_font_size).SetFontFamily("Verdana").SetData(stop->name)
            .SetFillColor(options_.underlayer_color).SetStrokeColor(options_.underlayer_color)
            .SetStrokeWidth(options_.underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    text.SetPosition(projector_({stop->coordinates}))
            .SetOffset({ options_.stop_label_offset})
            .SetFontSize(options_.stop_label_font_size).SetFontFamily("Verdana").SetData(stop->name)
            .SetFillColor("black");
    doc.Add(text_background);
    doc.Add(text);
}

maper::Maper::Maper(maper::RenderOptions options) : options_(options)
{

}

svg::Document maper::Maper::MakeMap(transport_catalogue::TransportCatalogue &transport_catalogue_) {
    svg::Document result_map;
    std::vector<const transport_catalogue::Bus*> buses_to_render;
    std::vector<const transport_catalogue::Stop*> stops_to_render;
    std::unordered_set<const transport_catalogue::Stop*> addedstops;
    std::vector <geo::Coordinates> points;
    for (const auto* bus : transport_catalogue_.GetAllBuses()) {
        if (!bus->stops.empty()) {
            buses_to_render.push_back(bus);
            for (const auto* stop : transport_catalogue_.GetAllStops()) {
                if (addedstops.count(stop) == 0) {
                    if (transport_catalogue_.HasBuses(stop)) {
                        addedstops.insert(stop);
                        stops_to_render.push_back(stop);
                        points.push_back(stop->coordinates);
                    }
                }
            }
        }
    }
    this->MakeSphereProjector(points);
    std::sort(buses_to_render.begin(), buses_to_render.end(),
              [](const transport_catalogue::Bus* lhs, const transport_catalogue::Bus* rhs)
              {return lhs->number < rhs->number;});
    std::sort(stops_to_render.begin(), stops_to_render.end(),
              [](const transport_catalogue::Stop* lhs, const transport_catalogue::Stop* rhs) { return lhs->name < rhs->name; });

    //создаём линии
    size_t color_counter = 0;
    for (const transport_catalogue::Bus* bus : buses_to_render) {
        this->AddRouteLines(result_map, bus, color_counter);
        ++color_counter;
    }
    //создаём названия автобусов
    color_counter = 0;
    for (const transport_catalogue::Bus* bus : buses_to_render) {
        this->AddRouteNames(result_map, bus, color_counter);
        ++color_counter;
    }
    std::unordered_set<const transport_catalogue::Stop*> used_stop;
    for (const transport_catalogue::Stop* stop : stops_to_render) {
        if (transport_catalogue_.GetBusesByStop(stop->name).empty() || used_stop.count(stop) > 0) {
            continue;
        }
        this->AddStopPoint(result_map, stop);
        used_stop.insert(stop);
    }
    std::unordered_set<const transport_catalogue::Stop*> used_stop_names;
    for (const transport_catalogue::Stop* stop : stops_to_render) {
        if (transport_catalogue_.GetBusesByStop(stop->name).empty() || used_stop_names.count(stop) > 0) {
            continue;
        }
        this->AddStopNames(result_map, stop);
        used_stop_names.insert(stop);

    }
    return result_map;
}



#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <optional>

#pragma once

namespace svg {


    struct Point {
        Point() = default;
        Point(double x, double y)
                : x(x)
                , y(y) {
        }
        double x = 0;
        double y = 0;
    };

    class Rgb {
    public:
        Rgb(uint8_t redd, uint8_t greenn, uint8_t bluee)
                : red(redd)
                , green(greenn)
                , blue(bluee){

        }
        Rgb() = default;

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    class Rgba {
    public:
        Rgba(uint8_t redd, uint8_t greenn, uint8_t bluee, double opacityy)
                : red(redd)
                , green(greenn)
                , blue(bluee)
                , opacity(opacityy) {

        }

        Rgba() = default;

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;
    };

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

    inline const Color NoneColor{"none"};

    struct ColorPrinter {
        std::ostream& out;

        void operator()(std::monostate) const {
            out << "none";
        }
        void operator()(std::string color) const {
            out << color;
        }
        void operator()(svg::Rgb color) const {
            out << "rgb(" << std::to_string(color.red) << ',' << std::to_string(color.green) << ',' << std::to_string(color.blue) << ')';
        }
        void operator()(svg::Rgba color) const {
            out << "rgba(" << std::to_string(color.red) << ',' << std::to_string(color.green) << ',' << std::to_string(color.blue) << ',' << color.opacity << ')';
        }
    };

    std::ostream& operator<<(std::ostream& out, const Color& color);

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    std::ostream& operator<<(std::ostream &out, StrokeLineCap code);

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream& operator<<(std::ostream &out, StrokeLineJoin code);

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeWidth(double stroke_width) {
            stroke_width_ = stroke_width;
            return AsOwner();
        }
        Owner& SetStrokeLineCap(StrokeLineCap stroke_line_cap) {
            stroke_line_cap_ = stroke_line_cap;
            return AsOwner();
        }
        Owner& SetStrokeLineJoin(StrokeLineJoin stroke_line_join) {
            stroke_line_join_ = stroke_line_join;
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                out << " fill=\""sv << *fill_color_ << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv << *stroke_color_ << "\""sv;
            }
            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (stroke_line_cap_) {
                out << " stroke-linecap=\""sv;
                if (stroke_line_cap_ == StrokeLineCap::BUTT) {
                    out << "butt";
                }
                if (stroke_line_cap_ == StrokeLineCap::ROUND) {
                    out << "round";
                }
                if (stroke_line_cap_ == StrokeLineCap::SQUARE) {
                    out << "square";
                }
                out << "\""sv;
            }
            if (stroke_line_join_) {
                out << " stroke-linejoin=\""sv;
                if (stroke_line_join_ == StrokeLineJoin::ARCS) {
                    out << "arcs";
                }
                if (stroke_line_join_ == StrokeLineJoin::BEVEL) {
                    out << "bevel";
                }
                if (stroke_line_join_ == StrokeLineJoin::MITER) {
                    out << "miter";
                }
                if (stroke_line_join_ == StrokeLineJoin::MITER_CLIP) {
                    out << "miter-clip";
                }
                if (stroke_line_join_ == StrokeLineJoin::ROUND) {
                    out << "round";
                }
                out << "\""sv;
            }
        }

    private:
        Owner& AsOwner() {
            // static_cast безопасно преобразует *this к Owner&,
            // если класс Owner — наследник PathProps
            return static_cast<Owner&>(*this);
        }


        std::optional<StrokeLineCap> stroke_line_cap_;
        std::optional<StrokeLineJoin> stroke_line_join_;
        std::optional<double> stroke_width_;
        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
    };

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
    struct RenderContext {
        RenderContext(std::ostream& out)
                : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
                : out(out)
                , indent_step(indent_step)
                , indent(indent) {
        }

        RenderContext Indented() const {
            return {out, indent_step, indent + indent_step};
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
    class Polyline final: public Object, public PathProps<Polyline>{
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);

        /*
         * Прочие методы и данные, необходимые для реализации элемента <polyline>
         */
    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> points_ = {};
    };

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
    class Text final: public Object, public PathProps<Text> {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);

        // Прочие данные и методы, необходимые для реализации элемента <text>
    private:
        void RenderObject(const RenderContext& context) const override;

        Point pos_ {0,0};
        Point offset_ {0, 0};
        uint32_t size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_ {};
    };

    /*
     * интерфейс objectcontainer
     */

    class ObjectContainer {
    public:
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

        template <typename Obj>
        void Add(Obj obj) {
            AddPtr(std::make_unique<Obj>(std::move(obj)));
        }

        virtual ~ObjectContainer() = default;
    };

    /*
     * интерфейс drawable
     */

    class Drawable {
    public:
        virtual void Draw(ObjectContainer& container) const = 0;
        virtual ~Drawable() = default;
    };

    /*
 *
 * класс документ, позволяет хранить в себе объекты типа Object
 */
    class Document : public ObjectContainer{
    public:
        /*
         Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
         Пример использования:
         Document doc;
         doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
        */

        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(std::unique_ptr<Object>&& obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

        // Прочие методы и данные, необходимые для реализации класса Document
    private:
        std::vector<std::unique_ptr<Object>> objects_;
    };

    /*
     * макрос для рисования звезды
     */

    namespace {
        svg::Polyline CreateStar(svg::Point center, double outer_rad, double inner_rad, int num_rays) {
            using namespace svg;
            Polyline polyline;
            for (int i = 0; i <= num_rays; ++i) {
                double angle = 2 * M_PI * (i % num_rays) / num_rays;
                polyline.AddPoint({center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle)});
                if (i == num_rays) {
                    break;
                }
                angle += M_PI / num_rays;
                polyline.AddPoint({center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle)});
            }
            return polyline;
        }

    }


}  // namespace svg

namespace shapes {

    class Triangle : public svg::Drawable {
    public:
        Triangle(svg::Point p1, svg::Point p2, svg::Point p3)
                : p1_(p1)
                , p2_(p2)
                , p3_(p3) {
        }

        // Реализует метод Draw интерфейса svg::Drawable
        void Draw(svg::ObjectContainer& container) const override {
            container.Add(svg::Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
        }

    private:
        svg::Point p1_, p2_, p3_;
    };

    class Star : public svg::Drawable {
    public:
        Star(svg::Point center, double outer_radius, double inner_radius, int num_rays)
                : center_(center)
                , outer_radius_(outer_radius)
                , inner_radius_(inner_radius)
                , num_rays_(num_rays) {
        }

        void Draw(svg::ObjectContainer& container) const override {
            container.Add(svg::CreateStar(center_, outer_radius_, inner_radius_, num_rays_).SetFillColor("red").SetStrokeColor("black"));
        }

    private:
        svg::Point center_;
        double outer_radius_, inner_radius_;
        int num_rays_;
    };

    class Snowman : public svg::Drawable {
    public:
        Snowman(svg::Point head_center, double head_radius)
                : head_center_(head_center)
                , head_radius_(head_radius) {

        }
        void Draw(svg::ObjectContainer& container) const override {
            container.Add(svg::Circle().SetCenter({head_center_.x, (head_center_.y + head_radius_ * 5)}).SetRadius(head_radius_*2).SetFillColor("rgb(240,240,240)").SetStrokeColor("black"));
            container.Add(svg::Circle().SetCenter({head_center_.x, (head_center_.y + head_radius_ * 2)}).SetRadius(head_radius_*1.5).SetFillColor("rgb(240,240,240)").SetStrokeColor("black"));
            container.Add(svg::Circle().SetCenter({head_center_.x, (head_center_.y)}).SetRadius(head_radius_).SetFillColor("rgb(240,240,240)").SetStrokeColor("black"));
        }
    private:
        svg::Point head_center_;
        double head_radius_;
    };

} // namespace shapes
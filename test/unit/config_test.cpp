#include <celengine/property.h>
#include <iostream>
#include <memory>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace celestia::engine;

class ConfigReader : IConfigUpdater
{
 public:
    ConfigReader(std::shared_ptr<Config> cfg) : IConfigUpdater(std::move(cfg)) {}
    void read();
};

void ConfigReader::read()
{
    beginUpdate();
        set("Distance", new Value(10.18));
        set("Name",     new Value("foobar"));
        set("Visible",  new Value(true));
    endUpdate();
}

class Application
{
    std::shared_ptr<Config>  m_cfg;

 public:
    Property<double> p1;
    NumericProperty  distance;
    StringProperty   name, type;
    BooleanProperty  visible;

    Application(const std::shared_ptr<Config> &cfg):
        m_cfg     (cfg),
        p1        (Property<double>(cfg, "P1", 10.5)),
        distance  (NumericProperty(cfg, "distance", 55.1, [](const double &v){ return v > 0 ? -v : v; })),
        name      (StringProperty(cfg, "name", "baz")),
        type      (StringProperty(cfg, "type", "baz")),
        visible   (BooleanProperty(cfg, "visible", false))
    {
    }
};

TEST_CASE("Configuration", "[Configuration]")
{
    auto c = std::make_shared<Config>();
    ConfigReader config_reader(c);
    config_reader.read();
    SECTION("Properties")
    {
        Application app(c);
        REQUIRE(app.p1() == 10.5);
        REQUIRE(app.distance() == -10.18);
        REQUIRE(app.name() == "foobar");
        REQUIRE(app.type() == "baz");
        REQUIRE(app.visible() == true);
        REQUIRE(app.p1 == 10.5);
        REQUIRE(app.distance == -10.18);
        REQUIRE(app.name == "foobar");
        REQUIRE(app.type == "baz");
        REQUIRE(app.visible == true);
    }
    SECTION("Configuration")
    {
        auto *v = c->find("Name");
        REQUIRE(v->getType() == Value::StringType);
        REQUIRE(v->getString() == "foobar");
    }
    SECTION("Update Configuration")
    {
        Application app(c);
        REQUIRE(app.visible() == true);
        app.visible = false;
        REQUIRE(app.visible() == false);
    }
}

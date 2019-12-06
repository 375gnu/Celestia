#pragma once

#include <vector>
#include <string>
#include <celutil/color.h>
#include <Eigen/Core>

class Overlay;

namespace celestia
{
class TextBlock
{
 public:
    TextBlock(const Overlay&, const Eigen::Vector3f&, const Color&);
    TextBlock() = delete;
    ~TextBlock() = default;
    TextBlock(const TextBlock&) = delete;
    TextBlock(TextBlock&&) = delete;
    TextBlock& operator=(const TextBlock&) = delete;
    TextBlock& operator=(TextBlock&&) = delete;

    void render() const;
    bool add(const std::string&);
    const char* getError() const;

 private:
    struct Block
    {
        std::string str;
        Color       color;
    };

    bool ParseMarkup(const std::string&);

    std::vector<Block> m_blocks;
    const Overlay     &m_overlay;
    Color              m_defaultColor;
    Eigen::Vector3f    m_position   { Eigen::Vector3f::Zero() };
    const char        *m_error      { "unknown error" };
    size_t             m_errorPos   { 0 };
};
};

#pragma once
#include "Control.h"
#include <d2d1_1.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <atomic>
#include <mutex>
#include <string>

namespace CUI {

using Microsoft::WRL::ComPtr;

/**
 * @brief 图片呈现的具体渲染类型模式。
 * Avatar: 圆形剪裁的个性头像。
 * FileIcon: 带有扩展名标识的小文档图标占位框。
 * StatusBadge: 内嵌小文本的圆形高亮状态徽章。
 * Custom: 从资源文件加载的自定义位图。
 * DynamicBitmap: 用于实时内存流绘制的高速更新动态位图纹理。
 * FileSource: 绑定自操作系统磁盘物理文件的解码图像。
 */
enum class ImageType {
    Avatar,        // 个性头像渲染样式
    FileIcon,      // 文档文件图标渲染样式
    StatusBadge,   // 状态徽章角标渲染样式
    Custom,        // 自定义资源图片渲染样式
    DynamicBitmap, // 动态内存流位图纹理样式
    FileSource     // 物理文件图像源解码样式
};

/**
 * @brief 图像缩放与比例拉伸拉缩填充规则。
 * None: 保持原图真实像素大小不拉伸。
 * Fill: 强行拉伸充满整个边界，可能破坏原图长宽宽高比例。
 * Uniform: 等比例缩放，并保证全图都能够显示在边界内（两边可能留白）。
 * UniformToFill: 等比例缩放，并保证填满整个边界（多余截断）。
 */
enum class Stretch {
    None,          // 无拉伸直接呈现
    Fill,          // 填充整个边界区（非等比例）
    Uniform,       // 等比例向内缩放充满
    UniformToFill  // 等比例向外扩张裁剪填充
};

/**
 * @brief 图片渲染控件（Image）。
 * 承载多种图片类型的绘制，支持通过 WIC 引擎异步解码磁盘物理图片文件（PNG, JPEG, GIF 等），
 * 并提供用于高频硬件像素流实时渲染（1000+ FPS 监控流）的 Zero-Copy 动态位图绑定 API。
 */
class Image : public Control {
public:
    Image();
    Image(ImageType type, const std::string& text = "");
    Image(ImageType type, const std::string& text, D2D1_COLOR_F color);
    virtual ~Image() = default;

    virtual const char* GetClassName() const override { return "Image"; } // 获取类名
    virtual Value GetProperty(PropertyId id) const override; // 反射获取属性值
    virtual bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值

    virtual Size Measure(Size availableSize) override; // 测算图片包络或首选尺寸大小
    virtual void OnRender(GraphicsContext& ctx) override; // 绘制图片位图（根据拉伸规则）或绘制占位头像、徽章及文件占位块
    virtual void OnThemeChanged() override; // 响应主题更改，清空旧位图缓存
    virtual bool OnAnimationTick() override; // 驱动动态视频流淡入或缩放动画步进
    virtual bool HasSelfAnimation() const override; // 检查是否正处于图片过渡动画中

    void SetImageType(ImageType type); // 更改图片渲染模式并触发重绘
    ImageType GetImageType() const { return m_imageType; } // 获取图片渲染模式
    void SetBadgeText(const std::string& text) { m_badgeText = text; } // 设定状态徽章内部显示的文字符号
    void SetBadgeColor(D2D1_COLOR_F color) { m_badgeColor = color; } // 设定状态徽章的填充背景色

    bool SetSource(const std::string& path); // 传入 UTF-8 编码的磁盘物理路径以异步加载解码图像（支持 SVG / PNG / BMP 等）
    const std::string& GetSource() const { return m_sourcePath; } // 获取加载的文件路径
    bool LoadFromMemory(const void* bytes, size_t size); // 从内存二进制字节数据中同步解码并建立位图
    void ClearSource(); // 清空当前绑定的图像源并标记重绘

    void SetStretch(Stretch stretch); // 更改长宽比拉伸填充规则
    Stretch GetStretch() const { return m_stretch; } // 获取拉伸规则

    int GetPixelWidth() const { return static_cast<int>(m_bmpWidth); } // 获取已解码图像的实际物理像素宽度 (px)
    int GetPixelHeight() const { return static_cast<int>(m_bmpHeight); } // 获取已解码图像的实际物理像素高度 (px)
    bool HasBitmap() const { return m_wicBitmap || m_d2dBitmap; } // 判断当前是否已装载并缓载了有效的位图纹理
    const std::string& GetLoadError() const { return m_loadError; } // 获取最近一次解码发生的报错描述

    bool InitDynamicBitmap(ID2D1DeviceContext* ctx, UINT width, UINT height); // 初始化并开辟硬件动态 BGRA 纹理缓冲区以作高频更新流
    void UpdatePixelBuffer(const uint32_t* bgraPixelData, UINT width, UINT height, UINT pitch = 0); // 线程安全地直接向硬件位图写入新的物理图像像素流
    void SetBitmap(ID2D1Bitmap1* bitmap); // 直接共享并绑定一个外部已经实例化的 ID2D1Bitmap1 纹理指针

private:
    IWICImagingFactory2* EnsureWicFactory(); // 确保 WIC 解码工厂初始化就绪
    bool DecodeFrame(IWICBitmapDecoder* decoder); // 执行解码图像帧的核心解码逻辑
    void ReleaseDecoded(); // 销毁并释放解码缓存
    void EnsureD2dBitmap(GraphicsContext& ctx); // 确保将解码出的 WIC 位图转换写入 Direct2D 硬件显存中
    Rect DestRectFor(float srcW, float srcH) const; // 根据拉伸规格计算出图像在画布中应占用的最终像素位置矩形
    void DrawFilePlaceholder(GraphicsContext& ctx); // 绘制默认的文档文件类型图标外观占位底盘

    ImageType m_imageType = ImageType::Avatar;          // 图像呈现类型，默认设为头像模式
    Stretch m_stretch = Stretch::Uniform;               // 拉伸比例规则，默认设为 Uniform 等比充满
    std::string m_badgeText = "UI";                     // 徽章文字缓存
    std::string m_sourcePath;                           // 图片文件在磁盘上的物理路径
    std::string m_loadError;                            // 解码异常报错描述
    D2D1_COLOR_F m_badgeColor{};                        // 徽章背景色

    ComPtr<IWICImagingFactory2> m_wicFactory;           // WIC 图片工厂组件指针
    ComPtr<IWICBitmap> m_wicBitmap;                     // 解码生成的 CPU 端 WIC 位图对象
    ComPtr<ID2D1Bitmap1> m_d2dBitmap;                   // 转换到显存端的 D2D 硬件位图对象
    ComPtr<ID2D1Device> m_boundDevice;                  // 位图当前关联绑定的渲染显卡设备实例
    UINT m_bmpWidth = 0;                                // 图像像素高精宽限制
    UINT m_bmpHeight = 0;                               // 图像像素高精高限制

    std::vector<uint32_t> m_pendingPixelBuffer;         // 线程安全写入所用的双缓冲动态像素行数据
    std::mutex m_bufferMutex;                           // 双缓冲读写互斥锁
    std::atomic<bool> m_hasPendingUpdate{ false };      // 标识主渲染线程是否需要将缓冲数据同步上传至显存
};

} // namespace CUI

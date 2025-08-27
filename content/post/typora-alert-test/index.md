---
title: "Typora风格警告框渲染测试"
description: "测试Typora风格警告框的渲染效果，包括NOTE、TIP、IMPORTANT、WARNING、CAUTION等类型"
slug: "typora-alert-test"
date: 2025-01-20
image: 
categories:
    - 测试
tags:
    - Typora
    - 警告框
    - Markdown
weight: 1
---

# Typora风格警告框渲染测试

本页面用于测试Typora风格警告框的渲染效果。以下展示了五种不同类型的警告框：

## NOTE 类型警告框

> [!NOTE]
> 这是一个NOTE类型的警告框。用于显示一般性的注意事项和说明信息。
> 
> 支持多行内容，可以包含**粗体**、*斜体*等Markdown格式。

## TIP 类型警告框

> [!TIP]
> 这是一个TIP类型的警告框。用于提供有用的提示和建议。
> 
> 例如：使用快捷键 `Ctrl+C` 可以快速复制内容。

## IMPORTANT 类型警告框

> [!IMPORTANT]
> 这是一个IMPORTANT类型的警告框。用于强调重要信息。
> 
> 重要信息通常需要用户特别关注，比如：
> - 关键的配置步骤
> - 必须遵守的规则
> - 影响系统行为的设置

## WARNING 类型警告框

> [!WARNING]
> 这是一个WARNING类型的警告框。用于警告可能的风险或问题。
> 
> ⚠️ 执行此操作前请确保已备份重要数据！

## CAUTION 类型警告框

> [!CAUTION]
> 这是一个CAUTION类型的警告框。用于提醒用户需要谨慎操作。
> 
> 请仔细阅读文档后再进行操作，避免造成不可逆的损失。

## 自定义标题的警告框

> [!NOTE] 自定义标题
> 警告框支持自定义标题，只需在类型后添加空格和标题文本即可。

> [!TIP] 专业提示
> 这个功能让警告框更加灵活，可以根据具体内容设置合适的标题。

## 包含代码的警告框

> [!IMPORTANT] 代码示例
> 警告框中也可以包含代码块：
> 
> ```javascript
> function showAlert(message) {
>     alert(message);
> }
> ```
> 
> 以及行内代码：`console.log('Hello World')`

## 包含链接的警告框

> [!NOTE] 相关链接
> 更多信息请参考：
> - [Hugo官方文档](https://gohugo.io/)
> - [Markdown语法指南](https://www.markdownguide.org/)
> - [Typora官网](https://typora.io/)

---

## 技术实现说明

本警告框功能通过以下技术实现：

1. **CSS样式**：在 `assets/scss/custom.scss` 中定义了完整的警告框样式
2. **JavaScript解析**：通过 `assets/js/typora-alerts.js` 解析 `>[!TYPE]` 语法
3. **Hugo配置**：启用了 `unsafe: true` 允许HTML渲染

### 支持的警告框类型

| 类型 | 语法 | 用途 |
|------|------|------|
| NOTE | `>[!NOTE]` | 一般注意事项 |
| TIP | `>[!TIP]` | 有用提示 |
| IMPORTANT | `>[!IMPORTANT]` | 重要信息 |
| WARNING | `>[!WARNING]` | 警告信息 |
| CAUTION | `>[!CAUTION]` | 谨慎提醒 |

### 使用方法

1. 在Markdown文件中使用 `>[!TYPE]` 语法
2. 可选择性添加自定义标题：`>[!TYPE] 自定义标题`
3. 在下一行开始编写警告框内容
4. 支持多行内容和Markdown格式

> [!TIP] 最佳实践
> 建议根据内容的重要程度和性质选择合适的警告框类型，以提供更好的用户体验。
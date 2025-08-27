/**
 * 自定义TypeScript文件
 * 用于加载Typora警告框功能
 */

// 导入Typora警告框脚本
import '../js/typora-alerts.js';

// 确保在DOM加载完成后初始化
document.addEventListener('DOMContentLoaded', () => {
    console.log('Typora alerts initialized');
});
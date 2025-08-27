/**
 * Typora风格警告框解析器
 * 解析 >[!NOTE] 等语法并转换为HTML警告框
 */

// 定义警告框类型和对应的图标
const alertTypes = {
    'NOTE': { icon: 'ℹ️', class: 'alert-note' },
    'TIP': { icon: '💡', class: 'alert-tip' },
    'IMPORTANT': { icon: '❗', class: 'alert-important' },
    'WARNING': { icon: '⚠️', class: 'alert-warning' },
    'CAUTION': { icon: '🚨', class: 'alert-caution' }
};

// 定义警告框标题
const alertTitles = {
    'NOTE': 'Note',
    'TIP': 'Tip', 
    'IMPORTANT': 'Important',
    'WARNING': 'Warning',
    'CAUTION': 'Caution'
};
    
    function createAlertElement(alertType, titleContent, originalBlockquote) {
        const alertDiv = document.createElement('div');
        alertDiv.className = `typora-alert ${alertTypes[alertType].class}`;
        
        // 创建标题
        const titleDiv = document.createElement('div');
        titleDiv.className = 'alert-title';
        titleDiv.textContent = titleContent || alertTitles[alertType];
        
        // 创建内容容器
        const contentDiv = document.createElement('div');
        contentDiv.className = 'alert-content';
        
        // 处理blockquote中的其他内容
        const paragraphs = originalBlockquote.querySelectorAll('p');
        
        // 跳过第一个段落（包含[!TYPE]语法），处理其余内容
        for (let i = 1; i < paragraphs.length; i++) {
            const clonedP = paragraphs[i].cloneNode(true);
            contentDiv.appendChild(clonedP);
        }
        
        // 如果第一个段落除了[!TYPE]还有其他内容，也要包含进来
        const firstP = paragraphs[0];
        if (firstP) {
            const text = firstP.textContent.trim();
            const match = text.match(/^\[!\w+\]\s*(.+)$/s);
            if (match && match[1].trim()) {
                const newP = document.createElement('p');
                newP.textContent = match[1].trim();
                contentDiv.insertBefore(newP, contentDiv.firstChild);
            }
        }
        
        // 如果没有内容，创建一个空的段落
        if (contentDiv.children.length === 0) {
            const emptyP = document.createElement('p');
            contentDiv.appendChild(emptyP);
        }
        
        alertDiv.appendChild(titleDiv);
        alertDiv.appendChild(contentDiv);
        
        return alertDiv;
    }
    
    function processBlockquote(blockquote) {
        const firstParagraph = blockquote.querySelector('p');
        if (!firstParagraph) {
            return false;
        }

        const text = firstParagraph.textContent.trim();
        
        // 匹配 [!TYPE] 语法
        const alertMatch = text.match(/^\[!(\w+)\]\s*(.*)$/s);
        if (!alertMatch) {
            return false;
        }

        const alertType = alertMatch[1].toUpperCase();
        const alertContent = alertMatch[2].trim();
        
        // 检查是否是支持的警告类型
        if (!alertTypes[alertType]) {
            return false;
        }

        // 创建警告框HTML
        const alertElement = createAlertElement(alertType, alertContent, blockquote);
        
        // 替换原始blockquote
        blockquote.parentNode.replaceChild(alertElement, blockquote);
        return true;
    }
    
    function parseAlerts() {
        // 查找所有的blockquote元素
        const blockquotes = document.querySelectorAll('blockquote');
        let processedCount = 0;
        
        // 转换为数组以避免在遍历时修改DOM导致的问题
        const blockquoteArray = Array.from(blockquotes);
        
        blockquoteArray.forEach(blockquote => {
            if (processBlockquote(blockquote)) {
                processedCount++;
            }
        });
        
        console.log(`Processed ${processedCount} alert boxes from ${blockquoteArray.length} blockquotes`);
    }
    
    function init() {
        console.log('Typora Alerts initializing...');
        console.log('Document ready state:', document.readyState);
        
        // 立即尝试解析
        parseAlerts();
        
        // DOM加载完成后再次解析
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', function() {
                console.log('DOMContentLoaded event fired, parsing alerts...');
                parseAlerts();
            });
        }
        
        // 页面完全加载后解析
        window.addEventListener('load', function() {
            console.log('Window load event fired, parsing alerts...');
            parseAlerts();
        });
        
        // 延迟解析，确保所有动态内容都已加载
        setTimeout(function() {
            console.log('Timeout 500ms, parsing alerts...');
            parseAlerts();
        }, 500);
        setTimeout(function() {
            console.log('Timeout 2000ms, parsing alerts...');
            parseAlerts();
        }, 2000);
    }
    
// 创建TyporaAlerts对象
const TyporaAlerts = {
    init: init,
    parseAlerts: parseAlerts
};

// 导出到全局作用域
window.TyporaAlerts = TyporaAlerts;

// 立即初始化
init();

// 导出模块（用于TypeScript导入）
export default TyporaAlerts;
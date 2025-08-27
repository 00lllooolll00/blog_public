// Custom TypeScript for Hugo Theme Stack
// This file is automatically included in the build process

// Import and initialize Typora alerts
import '../js/typora-alerts.js';

// Initialize custom functionality when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    console.log('Custom scripts loaded');
    
    // Initialize Typora alerts if the function exists
    if (typeof window.TyporaAlerts !== 'undefined') {
        window.TyporaAlerts.init();
        console.log('Typora alerts initialized');
    }
});
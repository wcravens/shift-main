/*
* general js functions that useful
*/
// format number to int
function numInt(num) {
    return parseFloat(num).toLocaleString(undefined, { maximumFractionDigits: 0, minimumFractionDigits: 0 });
}

// format number to float
function numFloat(num) {
    return parseFloat(num).toLocaleString(undefined, { maximumFractionDigits: 2, minimumFractionDigits: 2 });
}

// function to check if the device is a touch device
function isTouchDevice() {
    return true == ("ontouchstart" in window || window.DocumentTouch && document instanceof DocumentTouch);
}
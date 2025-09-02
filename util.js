export function get_sub(text) {
    return "<sub>" + text + "</sub>";
}
export function get_sup(text) {
    return "<sup>" + text + "</sup>";
}
export function range(start, end) {
    let result = [];
    let entry = start;
    while (entry < end) {
        result.push(entry);
        entry += 1;
    }
    return result;
}

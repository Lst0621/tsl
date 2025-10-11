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
export function create_2d_array(m, n, a) {
    return Array.from({ length: m }, () => Array(n).fill(a));
}
export function random_fill(array, threshold) {
    let l1 = array.length;
    let l2 = array[0].length;
    let new_array = [];
    for (let i = 0; i < l1; i++) {
        let row = [];
        for (let j = 0; j < l2; j++) {
            if (Math.random() < threshold) {
                row.push(1);
            }
            else {
                row.push(0);
            }
        }
        new_array.push(row);
    }
    return new_array;
}

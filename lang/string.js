export function get_all_prefixes(str) {
    const prefixes = [];
    for (let i = 0; i <= str.length; i++) {
        prefixes.push(str.substring(0, i));
    }
    return prefixes;
}

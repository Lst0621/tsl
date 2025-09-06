export function get_sub(text: string): string {
    return "<sub>" + text + "</sub>"
}

export function get_sup(text: string): string {
    return "<sup>" + text + "</sup>"
}

export function range(start: number, end: number) {
    let result = []
    let entry = start
    while (entry < end) {
        result.push(entry)
        entry += 1
    }
    return result
}

export function create_2d_array<T>(m: number, n: number, a: T): T[][] {
    return Array.from({ length: m }, () => Array(n).fill(a));
}
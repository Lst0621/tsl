export function get_sub(text: string): string {
    return "<sub>" + text + "</sub>"
}

export function get_sup(text: string): string {
    return "<sup>" + text + "</sup>"
}

export function range(start: number, end: number, step: number = 1): number[] {
    let result = []
    let entry = start
    while (entry < end) {
        result.push(entry)
        entry += step
    }
    return result
}

export function create_2d_array<T>(m: number, n: number, a: T): T[][] {
    return Array.from({ length: m }, () => Array(n).fill(a));
}

export function random_fill(array: number[][], threshold: number): number[][] {
    let l1 = array.length;
    let l2 = array[0].length;
    let new_array: number[][] = [];
    for (let i: number = 0; i < l1; i++) {
        let row: number[] = [];
        for (let j: number = 0; j < l2; j++) {
            if (Math.random() < threshold) {
                row.push(1);
            } else {
                row.push(0);
            }
        }
        new_array.push(row);
    }
    return new_array;
}
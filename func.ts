export function always<T>(a: T): (...args: any[]) => T {
    return (...args: any[]) => a;
}

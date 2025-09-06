import {cartesian_product} from "../math/set.js";
import {get_sup} from "../util.js";

export function get_all_prefixes(str: string): string[] {
    const prefixes: string[] = [];
    for (let i = 0; i <= str.length; i++) {
        prefixes.push(str.substring(0, i));
    }
    return prefixes;
}

export function get_all_chars(s: string): string[] {
    return Array.from(new Set(s.split(""))).sort();
}

export function get_alphabet_from_strings(strs: string[]) {
    return Array.from(new Set(strs.map(get_all_chars).flat()))
}

export function concat_string_lists(s1: string[], s2: string[]): string[] {
    return Array.from(new Set(cartesian_product([s1, s2]).map(
        (s) => s[0] + s[1]))).sort((a, b) => (a.length - b.length) || a.localeCompare(b));
}

// TODO play with power
export function concat_same_string_lists_n_times(s1: string[], n: number) {
    let result = Array.from(s1)
    for (let i = 1; i < n; i++) {
        result = concat_string_lists(result, s1)
    }
    return result
}

export function concat_same_string_lists_leq_n_times(s1: string[], n: number) {
    let s_with_empty = Array.from(s1).concat("")
    return concat_same_string_lists_n_times(s_with_empty, n)
}

//TODO empty handling
export function get_sub_seq_regex(s: string) {
    let sigma_star = "&Sigma;" + get_sup("*")
    return sigma_star + s.split("").join(sigma_star) + sigma_star
}

export function get_regex_for_disalloweb_sub_seq(s: string[]) {
    return "(" + s.map(get_sub_seq_regex).join("&cup;") + ")" + get_sup("<mi>c</mi>")
}
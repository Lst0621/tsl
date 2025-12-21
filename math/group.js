import { are_co_prime, get_add_inverse_mod_n_function, get_add_mod_n_function, get_multiply_mod_n_function } from "./number.js";
import { cartesian_product } from "./set.js";
import { array_to_matrix, get_det } from "./matrix.js";
import { get_sup } from "../util.js";
import { array_eq } from "./math.js";
import { generate_semigroup, get_idempotent_power } from "./semigroup.js";
export function get_u_n(n) {
    let u = [];
    for (let i = 1; i <= n; i++) {
        if (are_co_prime(i, n)) {
            u.push(i);
        }
    }
    return u;
}
export function get_order(element, multiply, eq) {
    return get_idempotent_power(element, multiply, eq)[0];
}
export function get_primitive_roots(n) {
    if (n == 1) {
        return [1];
    }
    let u = get_u_n(n);
    let ret = [];
    for (let i = 0; i < u.length; i++) {
        let a = u[i];
        let g = generate_group([a], get_multiply_mod_n_function(n), (a, b) => a == b, n);
        if (g.length == u.length) {
            let idempotent_power = get_order(a, get_multiply_mod_n_function(n), (x, y) => x == y);
            console.log("Compare group size with idem power", idempotent_power, g.length);
            ret.push(a);
        }
    }
    return ret;
}
export function pad_permutations(p1, p2) {
    let l1 = p1.length;
    let l2 = p2.length;
    let l = Math.max(l1, l2);
    let p1_copy = Array.from(p1);
    let p2_copy = Array.from(p2);
    // fill
    for (let i = l1 + 1; i <= l; i++) {
        p1_copy.push(i);
    }
    for (let i = l2 + 1; i <= l; i++) {
        p2_copy.push(i);
    }
    return [p1_copy, p2_copy];
}
export function permutation_eq(p1, p2) {
    let l1 = p1.length;
    let l2 = p2.length;
    let l = Math.max(l1, l2);
    let padded = pad_permutations(p1, p2);
    let p1_copy = padded[0];
    let p2_copy = padded[1];
    return array_eq(p1_copy, p2_copy);
}
export function permutation_multiply(p1, p2) {
    let l1 = p1.length;
    let l2 = p2.length;
    let l = Math.max(l1, l2);
    let padded = pad_permutations(p1, p2);
    let p1_copy = padded[0];
    let p2_copy = padded[1];
    let ans = [];
    for (let i = 1; i <= l; i++) {
        let p2_i = p2_copy[i - 1];
        let p1_p2_i = p1_copy[p2_i - 1];
        ans.push(p1_p2_i);
    }
    return ans;
}
export function get_identity_permutation(n) {
    return Array(n).fill(0).map((_, i) => i + 1);
}
export function get_all_permutations(n) {
    let e = get_identity_permutation(n);
    let perm = e;
    let ans = [e];
    while (true) {
        let next = next_permutation(perm);
        if (next.every((val, i) => val === e[i])) {
            break;
        }
        perm = next;
        ans.push(perm);
    }
    return ans;
}
export function get_all_permutations_recursive(n) {
    if (n == 1) {
        return [[1]];
    }
    let per_n_minus_one = get_all_permutations_recursive(n - 1);
    let ans = [];
    for (let permutation of per_n_minus_one) {
        for (let i = 0; i <= n - 1; i++) {
            let cp = Array.from(permutation);
            cp.splice(n - 1 - i, 0, n);
            ans.push(cp);
        }
    }
    return ans;
}
export function is_cycle_valid(cycle) {
    if (cycle == null) {
        return false;
    }
    for (let i = 0; i < cycle.length; i++) {
        if (isNaN(cycle[i])) {
            return false;
        }
        for (let j = i + 1; j < cycle.length; j++) {
            if (cycle[j] == cycle[i]) {
                return false;
            }
        }
    }
    return true;
}
export function get_permutation_from_cycle(cycle) {
    if (cycle == null) {
        return [];
    }
    let num_element = cycle[0];
    for (let num of cycle) {
        num_element = Math.max(num, num_element);
    }
    let perm = get_identity_permutation(num_element);
    if (cycle.length == 1) {
        return perm;
    }
    for (let i = 0; i < cycle.length; i++) {
        let from = cycle[i];
        let to = (i + 1) < cycle.length ? cycle[i + 1] : cycle[0];
        perm[from - 1] = to;
    }
    return perm;
}
export function get_cycles_from_permutations(perm) {
    let visited = Array(perm.length).fill(0);
    let cycles = [];
    let cycle = [];
    let i = 0;
    while (true) {
        if (i == perm.length) {
            break;
        }
        if (visited[i] == true) {
            if (cycle.length != 0) {
                cycles.push(Array.from(cycle));
            }
            cycle = [];
            i = i + 1;
            continue;
        }
        let from = i + 1;
        let to = perm[i];
        visited[i] = true;
        cycle.push(from);
        i = to - 1;
    }
    return cycles;
}
export function get_permutation_parity(perm) {
    let cycles = get_cycles_from_permutations(perm);
    let parity = true;
    for (let cycle of cycles) {
        let is_cycle_odd = cycle.length % 2 == 0;
        parity = (parity && !is_cycle_odd) || (!parity && is_cycle_odd);
    }
    return parity;
}
export function get_arrow_string_from_cycle(cycle) {
    let arrow = "->";
    let ret = "(";
    ret += cycle.map(String).join(arrow);
    ret += arrow + cycle[0].toString() + ")";
    return ret;
}
export function get_string_from_cycle(cycle) {
    let join_str = cycle.some(value => value > 9) ? "," : "";
    return "(" + cycle.map(String).join(join_str) + ")";
}
export function per_to_arrow(perm) {
    let cycles = get_cycles_from_permutations(perm);
    let ret = cycles.filter((cycle) => cycle.length > 1).map(get_arrow_string_from_cycle).join("");
    if (ret.length == 0) {
        ret = "e";
    }
    return ret;
}
export function perm_to_str(perm) {
    let cycles = get_cycles_from_permutations(perm);
    let cycle_str = cycles.filter((cycle) => cycle.length > 1).map(get_string_from_cycle).join("");
    if (cycle_str.length == 0) {
        return "e";
    }
    return cycle_str;
}
function next_permutation(perm) {
    let next = Array.from(perm);
    next_permutation_in_place(next);
    return next;
}
function next_permutation_in_place(nums) {
    // Step 1: Find the first index `i` from the end where nums[i] < nums[i + 1]
    let i = nums.length - 2;
    while (i >= 0 && nums[i] >= nums[i + 1]) {
        i--;
    }
    if (i >= 0) {
        // Step 2: Find the first index `j` from the end where nums[j] > nums[i]
        let j = nums.length - 1;
        while (j >= 0 && nums[j] <= nums[i]) {
            j--;
        }
        // Step 3: Swap nums[i] and nums[j]
        [nums[i], nums[j]] = [nums[j], nums[i]];
    }
    // Step 4: Reverse the suffix starting at i + 1
    let left = i + 1;
    let right = nums.length - 1;
    while (left < right) {
        [nums[left], nums[right]] = [nums[right], nums[left]];
        left++;
        right--;
    }
}
export function get_all_dihedral(n) {
    return generate_group([[1, 0], [0, 1]], (a, b) => dihedral_multiply(a, b, n), array_eq, 0).sort((a, b) => ((a[1] == b[1]) ? a[0] - b[0] : a[1] - b[1]));
}
export function dihedral_multiply(a, b, n) {
    let r_a = a[0];
    let s_a = a[1];
    let r_b = b[0];
    let s_b = b[1];
    if (s_a == 0) {
        return [(r_a + r_b) % n, s_b];
    }
    else {
        return [(r_a + n - r_b) % n, 1 - s_b];
    }
}
export function dihedral_to_permutation(dihedral, n) {
    let perm = [];
    let r = [];
    for (let i = 2; i <= n; i++) {
        r.push(i);
    }
    r.push(1);
    for (let i = 0; i < dihedral[0]; i++) {
        perm = permutation_multiply(perm, r);
    }
    if (dihedral[1] == 1) {
        let s = [];
        for (let i = 1; i <= n; i++) {
            if (i == 1) {
                s.push(i);
            }
            else {
                s.push(n + 2 - i);
            }
        }
        perm = permutation_multiply(perm, s);
    }
    return perm;
}
export function dihedral_to_str(a) {
    let r = a[0];
    let s = a[1];
    if (r == 0 && s == 0) {
        return "e";
    }
    if (r == 0) {
        return "s";
    }
    return "r" + (r == 1 ? "" : get_sup(r.toString())) + (s == 0 ? "" : "s");
}
export function generate_group(generators, multiply, eq, limit) {
    // TODO, probably this is no longer needed
    return generate_semigroup(generators, multiply, eq, limit);
}
export function gen_general_linear_n_zm(n, m) {
    let gen = [];
    for (let j = 0; j < n * n; j++) {
        let elements = [];
        for (let i = 0; i < m; i++) {
            elements.push(i);
        }
        gen.push(elements);
    }
    return cartesian_product(gen).map(arr => array_to_matrix(arr, n, n)).filter(mat => get_det(mat, get_multiply_mod_n_function(m), get_add_mod_n_function(m), get_add_inverse_mod_n_function(m)) != 0);
}

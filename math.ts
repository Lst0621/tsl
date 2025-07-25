import {get_sup} from "./util.js";

export function gcd(a_in: number, b_in: number): number {
    let a: number = Math.abs(a_in);
    let b: number = Math.abs(b_in);
    if (a == 0 || b == 0) {
        return a + b
    }

    if (a == b) {
        return a
    }

    if (a > b) {
        let tmp: number = a
        a = b
        b = tmp
    }

    // a < b
    while (true) {
        let res = b % a
        if (res == 0) {
            return a
        }
        b = a
        a = res
    }
}

export function multiply_mod_n(a: number, b: number, n: number): number {
    return a * b % n;
}

export function get_multiply_mod_n_function(n: number) {
    return (a: number, b: number) => multiply_mod_n(a, b, n)
}

export function are_co_prime(a: number, b: number): boolean {
    return gcd(a, b) == 1
}

export function totient(n: number): number {
    let ans: number = 0
    for (let i = 1; i <= n; i++) {
        if (are_co_prime(i, n)) {
            ans += 1
        }
    }
    return ans
}

export function get_u_n(n: number): number[] {
    let u: number[] = []
    for (let i = 1; i <= n; i++) {
        if (are_co_prime(i, n)) {
            u.push(i)
        }
    }
    return u
}

export function get_primitive_roots(n: number) {
    if (n == 1) {
        return [1]
    }
    let u = get_u_n(n)
    let ret = []
    for (let i = 0; i <= u.length; i++) {
        let a = u[i]
        let g = generate_group([a], get_multiply_mod_n_function(n), (a, b) => a == b, n)
        if (g.length == u.length) {
            ret.push(a)
        }
    }
    return ret
}

export function pad_permutations(p1: number[], p2: number[]) {
    let l1 = p1.length;
    let l2 = p2.length;
    let l = Math.max(l1, l2);
    let p1_copy = Array.from(p1)
    let p2_copy = Array.from(p2)

    // fill
    for (let i = l1 + 1; i <= l; i++) {
        p1_copy.push(i)
    }
    for (let i = l2 + 1; i <= l; i++) {
        p2_copy.push(i)
    }
    return [p1_copy, p2_copy]
}

export function array_eq(a: number[], b: number[]): boolean {
    let len_a: number = a.length;
    let len_b: number = b.length;
    if (len_a != len_b) {
        return false;
    }
    for (let i = 0; i < len_a; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

export function permutation_eq(p1: number[], p2: number[]) {
    let l1 = p1.length;
    let l2 = p2.length;
    let l = Math.max(l1, l2);

    let padded = pad_permutations(p1, p2)
    let p1_copy = padded[0]
    let p2_copy = padded[1]
    return array_eq(p1_copy, p2_copy)
}

export function permutation_multiply(p1: number[], p2: number[]): number[] {
    let l1 = p1.length;
    let l2 = p2.length;
    let l = Math.max(l1, l2);

    let padded = pad_permutations(p1, p2)
    let p1_copy = padded[0]
    let p2_copy = padded[1]

    let ans: number[] = []
    for (let i = 1; i <= l; i++) {
        let p2_i = p2_copy[i - 1]
        let p1_p2_i: number = p1_copy[p2_i - 1]
        ans.push(p1_p2_i)
    }
    return ans
}

export function get_identity_permutation(n: number): number[] {
    return Array(n).fill(0).map((_, i) => i + 1);
}

export function get_all_permutations(n: number): number[][] {
    let e: number[] = get_identity_permutation(n)
    let perm: number[] = e
    let ans: number[][] = [e]
    while (true) {
        let next: number[] = next_permutation(perm)
        if (next.every((val, i) => val === e[i])) {
            break
        }
        perm = next
        ans.push(perm)
    }
    return ans
}

export function get_all_permutations_recursive(n: number): number[][] {
    if (n == 1) {
        return [[1]]
    }
    let per_n_minus_one = get_all_permutations_recursive(n - 1)
    let ans: number[][] = []
    for (let permutation of per_n_minus_one) {
        for (let i: number = 0; i <= n - 1; i++) {
            let cp: number[] = Array.from(permutation)
            cp.splice(n - 1 - i, 0, n)
            ans.push(cp)
        }
    }
    return ans
}

export function is_cycle_valid(cycle: number[]) {
    if (cycle == null) {
        return false
    }
    for (let i = 0; i < cycle.length; i++) {
        if (isNaN(cycle[i])) {
            return false
        }
        for (let j = i + 1; j < cycle.length; j++) {
            if (cycle[j] == cycle[i]) {
                return false
            }
        }
    }

    return true
}

export function get_permutation_from_cycle(cycle: number[]): number[] {
    if (cycle == null) {
        return []
    }
    let num_element: number = cycle[0]
    for (let num of cycle) {
        num_element = Math.max(num, num_element)
    }
    let perm: number[] = get_identity_permutation(num_element)
    if (cycle.length == 1) {
        return perm
    }
    for (let i = 0; i < cycle.length; i++) {
        let from = cycle[i]
        let to = (i + 1) < cycle.length ? cycle[i + 1] : cycle[0]
        perm[from - 1] = to
    }
    return perm
}

export function get_cycles_from_permutations(perm: number[]): number[][] {
    let visited = Array(perm.length).fill(0)
    let cycles: number[][] = []
    let cycle: number[] = []
    let i = 0
    while (true) {
        if (i == perm.length) {
            break
        }
        if (visited[i] == true) {
            if (cycle.length != 0) {
                cycles.push(Array.from(cycle))
            }
            cycle = []
            i = i + 1
            continue
        }
        let from: number = i + 1
        let to: number = perm[i]
        visited[i] = true
        cycle.push(from)
        i = to - 1
    }
    return cycles
}

export function get_permutation_parity(perm: number[]) {
    let cycles: number[][] = get_cycles_from_permutations(perm)
    let parity: boolean = true
    for (let cycle of cycles) {
        let is_cycle_odd = cycle.length % 2 == 0
        parity = (parity && !is_cycle_odd) || (!parity && is_cycle_odd)
    }
    return parity
}

export function get_arrow_string_from_cycle(cycle: number[]): string {
    let arrow = "->"
    let ret = "("
    ret += cycle.map(String).join(arrow)
    ret += arrow + cycle[0].toString() + ")"
    return ret
}

export function get_string_from_cycle(cycle: number[]): string {
    let join_str = cycle.some(value => value > 9) ? "," : ""
    return "(" + cycle.map(String).join(join_str) + ")"
}

export function per_to_arrow(perm: number[]): string {
    let cycles: number[][] = get_cycles_from_permutations(perm)
    let ret: string = cycles.filter((cycle) => cycle.length > 1).map(get_arrow_string_from_cycle).join("")
    if (ret.length == 0) {
        ret = "e"
    }
    return ret
}

export function perm_to_str(perm: number[]) {
    let cycles: number[][] = get_cycles_from_permutations(perm)
    let cycle_str: string = cycles.filter((cycle) => cycle.length > 1).map(get_string_from_cycle).join("")
    if (cycle_str.length == 0) {
        return "e"
    }
    return cycle_str
}

function next_permutation(perm: number[]): number[] {
    let next: number[] = Array.from(perm)
    next_permutation_in_place(next)
    return next
}

function next_permutation_in_place(nums: number[]): void {
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

export function get_all_dihedral(n: number): number[][] {
    return generate_group([[1, 0], [0, 1]], (a: number[], b: number[]) => dihedral_multiply(a, b, n), array_eq, 0).sort(
        (a, b) => ((a[1] == b[1]) ? a[0] - b[0] : a[1] - b[1]))
}

export function dihedral_multiply(a: number[], b: number[], n: number) {
    let r_a = a[0]
    let s_a = a[1]
    let r_b = b[0]
    let s_b = b[1]
    if (s_a == 0) {
        return [(r_a + r_b) % n, s_b]
    } else {
        return [(r_a + n - r_b) % n, 1 - s_b]
    }
}

export function dihedral_to_permutation(dihedral: number[], n: number) {
    let perm: number[] = []
    let r: number[] = []
    for (let i = 2; i <= n; i++) {
        r.push(i)
    }
    r.push(1)

    for (let i = 0; i < dihedral[0]; i++) {
        perm = permutation_multiply(perm, r)
    }

    if (dihedral[1] == 1) {
        let s: number[] = []
        for (let i = 1; i <= n; i++) {
            if (i == 1) {
                s.push(i);
            } else {
                s.push(n + 2 - i)
            }
        }
        perm = permutation_multiply(perm, s)
    }

    return perm
}

export function dihedral_to_str(a: number[]) {
    let r = a[0]
    let s = a[1]
    if (r == 0 && s == 0) {
        return "e"
    }
    if (r == 0) {
        return "s"
    }
    return "r" + (r == 1 ? "" : get_sup(r.toString())) + (s == 0 ? "" : "s")
}

// relations
export function is_reflexive(relation: boolean[][]) {
    let len = relation.length
    for (let i = 0; i < len; i++) {
        if (!relation[i][i]) {
            return false
        }
    }
    return true
}

export function is_symmetric(relation: boolean[][]) {
    let len = relation.length
    for (let i = 0; i < len; i++) {
        for (let j = i + 1; j < relation.length; j++) {
            if (relation[j][i] != relation[i][j]) {
                return false
            }
        }
    }
    return true
}

export function is_antisymmetric(relation: boolean[][]) {
    let len = relation.length
    for (let i = 0; i < len; i++) {
        for (let j = i + 1; j < relation.length; j++) {
            if (relation[j][i] && relation[i][j]) {
                return false
            }
        }
    }
    return true
}

export function is_transitive(relation: boolean[][]) {
    let len = relation.length
    for (let i = 0; i < relation.length; i++) {
        for (let j = 0; j < relation.length; j++) {
            if (!relation[i][j]) {
                continue
            }
            for (let k = 0; k < relation.length; k++) {
                if (relation[j][k]) {
                    if (!relation[i][k]) {
                        return false
                    }
                }
            }
        }
    }
    return true
}

export function is_equivalence(relation: boolean[][]) {
    return is_reflexive(relation) && is_symmetric(relation) && is_transitive(relation)
}

export function cartesian_product<T>(inputs: T[][]): T[][] {
    let all_combinations: T[][] = []
    cartesian_helper(inputs, [], all_combinations)
    return all_combinations;
}

function cartesian_helper<T>(inputs: T[][], l: T[], all_combinations: T[][]) {
    let index: number = l.length;
    let number_of_sets: number = inputs.length;
    if (number_of_sets == index) {
        all_combinations.push(Array.from(l));
        return;
    }
    for (let element of inputs[index]) {
        l.push(element)
        cartesian_helper(inputs, l, all_combinations)
        l.pop()
    }
}

export function generate_group<T>(generators: T[],
                                  multiply: (a: T, b: T) => T,
                                  eq: (a: T, b: T) => boolean,
                                  limit: number): T[] {
    let ret: T[] = Array.from(generators)
    let last_length: number = 0
    let current_length: number = ret.length
    console.log("generating elements of the group, size from " + last_length + " to " + current_length)
    while (last_length < current_length) {
        for (let i = 0; i < current_length; i++) {
            for (let j = last_length; j < current_length; j++) {
                let product_ij = multiply(ret[i], ret[j])
                if (!ret.some(ele => eq(ele, product_ij))) {
                    console.log("adding " + ret[i] + " " + ret[j])
                    ret.push(product_ij)
                }
            }
        }

        for (let i = 0; i < current_length; i++) {
            for (let j = last_length; j < current_length; j++) {
                let product_ji = multiply(ret[j], ret[i])
                if (!ret.some(ele => eq(ele, product_ji))) {
                    console.log("adding " + ret[j] + " " + ret[i])
                    ret.push(product_ji)
                }
            }
        }


        last_length = current_length
        current_length = ret.length
        console.log("generating elements of the group, size from " + last_length + " to " + current_length)
        if (limit > 0 && current_length > limit) {
            break;
        }
    }

    return ret
}

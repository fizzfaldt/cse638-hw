SOMETHING S[n]; //S[j] = Score(1,j)
Set Folds();
SOMETHING BestScore[n];

for (int j = n downto 1) {
    int l_1 = n-j-1; // Max \ell due to k
    if (l_1 < 1) {
        S[j] = 0;
    } else {
        spawn {
            int l_max = min(l_1, j-1);
            parallel for (int l = 1 to l_max) {
                ScoreOne[l] = P[j-l].hydrophobic && P[j+1+l].hydrophobic;
            }
            parallel_prefix_sum(ScoreOne, plus, 1, l_max);
        }
        spawn {
            parallel for (int k = j+1+1 to n) {
                //TODO: Store enough information to regen folds
                BestScore[k] = Score[j+1,k];
            }
            parallel_prefix_reverse_sum(BestScore, max_with_folds, j+1+1, n);
        }
        sync;
        int num_i = min(j-1, l_1);
        // Do not see how to parallelize.
        for (int i = j-1 downto j-num_i) {
            int k_min = 2j-i+1;
            int l = j-i;

            if (BestScore[k_min].score + ScoreOneFold[l] > S[j].score) {
                S[j].score = BestScore[k_min].score + ScoreOneFold[l];
                // Path is partial (linked list)
                S[j].path = (j,&BestScore[k_min].first);
            }
            Score[i,j] = S[j].clone(); // Adds at most num_i (max n) items
        }
        if (S[j].score > S[j+1].score) {
            S[j].path.reconstruct(); // <-- Stores full path
        } else {
            S[j] = S[j+1].clone(); // <-- Stores full path
        }
        delete S[j+1];
        deleterow Score[j+1,*] // Deletes at least num_i -1 items.
    }
    return S[1];
}

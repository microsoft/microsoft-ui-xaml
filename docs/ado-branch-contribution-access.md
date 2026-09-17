# Grant contribution access to an ADO branch

Use this procedure to grant contribution access to members of the required WinUI groups for a specific branch.

1. In Azure DevOps, open **Repos** > **Branches**.
2. Find the branch, select **More options** (`...`), and then select **Branch security**.
3. Select `[WinUI]\Contributors`.
4. Set **Contribute** to **Allow**.
5. Confirm that the permission remains **Allow** after the page refreshes.

Some developers are also members of `[WinUI]\Build Administrators`. If a developer is still denied access after
`[WinUI]\Contributors` is allowed, select `[WinUI]\Build Administrators` and set **Contribute** to **Allow** as well.

If the developer is still denied access, hover over the information icon next to their **Contribute** permission to
check where the permission is inherited from.

These changes apply to every member of the selected group on that branch. After the required changes are merged, set
**Contribute** back to **Deny** for each group that was temporarily changed.

If you cannot change branch security, ask a repository or project administrator who has **Manage permissions** for the
branch to make the change.

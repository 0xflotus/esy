open SolveUtils;

type t = {
  config: Config.t,
  availableNpmVersions:
    Hashtbl.t(string, list((NpmVersion.t, Yojson.Basic.json))),
  availableOpamVersions:
    Hashtbl.t(string, list((Types.opamConcrete, OpamFile.thinManifest))),
};

let getAvailableVersions = (~config: Config.t, ~cache: t, (name, source)) =>
  switch (source) {
  | Types.Github(user, repo, ref) => [`Github((user, repo, ref))]
  | Npm(semver) =>
    if (! Hashtbl.mem(cache.availableNpmVersions, name)) {
      Hashtbl.replace(
        cache.availableNpmVersions,
        name,
        NpmRegistry.getFromNpmRegistry(config, name),
      );
    };
    let available = Hashtbl.find(cache.availableNpmVersions, name);
    available
    |> List.sort(((va, _), (vb, _)) => NpmVersion.compare(va, vb))
    |> List.mapi((i, (v, j)) => (v, j, i))
    |> List.filter(((version, _json, _i)) =>
         NpmVersion.matches(semver, version)
       )
    |> List.map(((version, json, i)) => `Npm((version, json, i)));
  | Opam(semver) =>
    if (! Hashtbl.mem(cache.availableOpamVersions, name)) {
      Hashtbl.replace(
        cache.availableOpamVersions,
        name,
        OpamRegistry.getFromOpamRegistry(cache.config, name),
      );
    };
    let available =
      Hashtbl.find(cache.availableOpamVersions, name)
      |> List.sort(((va, _), (vb, _)) => OpamVersion.compare(va, vb))
      |> List.mapi((i, (v, j)) => (v, j, i));
    let matched =
      available
      |> List.filter(((version, _path, _i)) =>
           OpamVersion.matches(semver, version)
         );
    let matched =
      if (matched == []) {
        available
        |> List.filter(((version, _path, _i)) =>
             OpamVersion.matches(tryConvertingOpamFromNpm(semver), version)
           );
      } else {
        matched;
      };
    matched |> List.map(((version, path, i)) => `Opam((version, path, i)));
  | Git(_) => failwith("git dependencies are not supported")
  | LocalPath(p) => [`LocalPath(p)]
  };

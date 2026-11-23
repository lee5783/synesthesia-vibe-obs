#
#  P4-002-workflow-step.py
#  SynesthesiaVibeOBS
#
#  Created by lee5783 on 2026-11-23.
#

import sys

import yaml


def main():
    if len(sys.argv) != 4:
        sys.exit("usage: P4-002-workflow-step.py <workflow.yaml> <job id> <step name>")
    workflow_path, job_id, step_name = sys.argv[1:]
    with open(workflow_path, encoding="utf-8") as workflow_file:
        workflow = yaml.safe_load(workflow_file)
    for step in workflow["jobs"][job_id]["steps"]:
        if step.get("name") == step_name:
            sys.stdout.write(step["run"])
            return
    sys.exit(f"step '{step_name}' not found in job '{job_id}' of {workflow_path}")


if __name__ == "__main__":
    main()

Getting Started
===============

    # Start Minikube, if you haven't already
    minikube start
    
    # Apply namespaces
    kubectl apply -f manifests/env.yaml
    
    # Set a default namespace
    kubectl config set-context --current --namespace=dosa-dev
    
    # IMPORTANT: Will make Bazel publish images to Minikube instead of Docker (see above GCR vs local images)
    eval $(minikube docker-env)

    # Run the package job for Docker containers
    bazel run //pkg/zookeeper:latest
    
    # Build or update the stack:
    kubectl apply -f manifests/local/

    # Tear-down the stack:
    kubectl delete -f manifests/local/
 

### Minikube Visibility

To get a view of what is happening in Minikube, run the dashboard CLI and open the provided link in your browser:

    minikube dashboard
    
> *Minikube does not have load balancers!* 

To access the HTTP end-points, you need to use the NodePort method on the HTTP service: 

    minikube ip
    
Open that IP in your browser using the `nodePort` of the service, eg: `http://192.168.xx.xx:30080/`
